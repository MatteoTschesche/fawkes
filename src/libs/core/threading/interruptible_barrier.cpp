
/***************************************************************************
 *  interruptible_barrier.cpp - Interruptible Barrier
 *
 *  Created: Sat Jan 31 12:30:32 2009
 *  Copyright  2006-2009  Tim Niemueller [www.niemueller.de]
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL_WRE file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL_WRE file in the doc directory.
 */

#include <core/exceptions/system.h>
#include <core/macros.h>
#include <core/threading/interruptible_barrier.h>
#include <core/threading/mutex.h>
#include <core/threading/thread_list.h>
#include <core/threading/wait_condition.h>

namespace fawkes {

/// @cond INTERNALS
class InterruptibleBarrierData
{
public:
	unsigned int   threads_left;
	Mutex *        mutex;
	WaitCondition *waitcond;
	bool           own_mutex;

	InterruptibleBarrierData(Mutex *mutex)
	{
		if (mutex) {
			this->mutex = mutex;
			own_mutex   = false;
		} else {
			this->mutex = new Mutex();
			own_mutex   = true;
		}
		waitcond = new WaitCondition(this->mutex);
	}

	~InterruptibleBarrierData()
	{
		if (own_mutex)
			delete mutex;
		delete waitcond;
	}
};
/// @endcond

/** @class InterruptibleBarrier <core/threading/barrier.h>
 * A barrier is a synchronization tool which blocks until a given number of
 * threads have reached the barrier. This particular implementations allows for
 * giving a timeout after which the waiting is aborted.
 *
 * For general information when a barrier is useful see the Barrier class.
 *
 * Additionally to the general barrier features the InterruptibleBarrier::wait()
 * can be given a timeout after which the waiting is aborted.
 * Since the POSIX standard does not provide a timed wait for barriers this
 * implementation uses a Mutex and WaitCondition internally to achieve the
 * desired result.
 *
 * @see Barrier
 * @ingroup Threading
 * @author Tim Niemueller
 */

/** Constructor.
 * @param count the number of threads to wait for
 */
InterruptibleBarrier::InterruptibleBarrier(unsigned int count) : Barrier(count)
{
	_count = count;
	if (_count == 0) {
		throw Exception("Barrier count must be at least 1");
	}
	data_               = new InterruptibleBarrierData(NULL);
	data_->threads_left = 0;
	passed_threads_     = RefPtr<ThreadList>(new ThreadList());

	interrupted_                  = false;
	timeout_                      = false;
	num_threads_in_wait_function_ = 0;
}

/** Constructor with custom mutex.
 * Use this constructor only if you really know what you are doing. This constructor
 * allows to pass a mutex that is used internally for the barrier. Note that in
 * this case it is your duty to lock the mutex before the wait() and unlock
 * afterwards! It combines features of a barrier and a wait condition.
 * @param mutex Mutex to use
 * @param count the number of threads to wait for
 */
InterruptibleBarrier::InterruptibleBarrier(Mutex *mutex, unsigned int count) : Barrier(count)
{
	_count = count;
	if (_count == 0) {
		throw Exception("Barrier count must be at least 1");
	}
	data_               = new InterruptibleBarrierData(mutex);
	data_->threads_left = 0;
	passed_threads_     = RefPtr<ThreadList>(new ThreadList());

	interrupted_                  = false;
	timeout_                      = false;
	num_threads_in_wait_function_ = 0;
}

/** Invalid constructor.
 * This will throw an exception if called as it is illegal to copy
 * a barrier.
 * @param barrier to copy
 */
InterruptibleBarrier::InterruptibleBarrier(const InterruptibleBarrier &b) : Barrier()
{
	throw Exception("Barriers cannot be copied");
}

/** Invalid constructor.
 * This will throw an exception if called as it is illegal to copy
 * a barrier.
 * @param barrier to copy
 */
InterruptibleBarrier::InterruptibleBarrier(const InterruptibleBarrier *b) : Barrier()
{
	throw Exception("Barriers cannot be copied");
}

/** Invalid assignment operator.
 * This will throw an exception if called as it is illegal to assign
 * a barrier.
 * @param barrier to copy
 */
InterruptibleBarrier &
InterruptibleBarrier::operator=(const InterruptibleBarrier &b)
{
	throw Exception("Barriers cannot be assigned");
}

/** Invalid assignment operator.
 * This will throw an exception if called as it is illegal to assign
 * a barrier.
 * @param barrier to copy
 */
InterruptibleBarrier &
InterruptibleBarrier::operator=(const InterruptibleBarrier *b)
{
	throw Exception("Barriers cannot be assigned");
}

/** Destructor */
InterruptibleBarrier::~InterruptibleBarrier()
{
	delete data_;
}

/** Get a list of threads that passed the barrier.
 * The list contains the threads that passed the barrier. With some book keeping
 * outside of the barrier you can determine which threads you expected at the
 * barrier but did not pass it.
 * @return refptr to list of threads that passed the barrier.
 */
RefPtr<ThreadList>
InterruptibleBarrier::passed_threads()
{
	return passed_threads_;
}

/** Interrupt the barrier.
 * This will cause all threads currently waiting on the barrier to
 * throw an exception and no further thread will wait.
 * You have to call reset() the before you use this barrier
 * the next time.
 */
void
InterruptibleBarrier::interrupt() noexcept
{
	if (likely(data_->own_mutex))
		data_->mutex->lock();
	interrupted_ = true;
	data_->waitcond->wake_all();
	if (likely(data_->own_mutex))
		data_->mutex->unlock();
}

/** Clears the barrier.
 * Call this method when you want to use the barrier the next time after
 * an interrupt or timeout occured. Make sure all threads that should have
 * passed the barrier the last time did pass it.
 */
void
InterruptibleBarrier::reset() noexcept
{
	if (likely(data_->own_mutex))
		data_->mutex->lock();
	interrupted_        = false;
	timeout_            = false;
	data_->threads_left = _count;
	passed_threads_.clear();
	if (likely(data_->own_mutex))
		data_->mutex->unlock();
}

/** Wait for other threads.
 * This method will block until as many threads have called wait as you have
 * given count to the constructor. Note that if the barrier is interrupted or
 * times out you need to call reset() to get the barrier into a re-usable state.
 * It is your duty to make sure that all threads using the barrier are in a
 * cohesive state.
 * @param timeout_sec relative timeout in seconds, added to timeout_nanosec
 * @param timeout_nanosec timeout in nanoseconds
 * @return true, if the barrier was properly reached, false if the barrier timeout
 * was reached and the wait did not finish properly.
 * @exception InterruptedException thrown if the barrier was forcefully interrupted
 * by calling interrupt().
 */
bool
InterruptibleBarrier::wait(unsigned int timeout_sec, unsigned int timeout_nanosec)
{
	if (likely(data_->own_mutex))
		data_->mutex->lock();
	num_threads_in_wait_function_++;

	if (data_->threads_left == 0) {
		// first to come
		timeout_ = interrupted_ = wait_at_barrier_ = false;
		data_->threads_left                        = _count;
		passed_threads_->clear();
	} else {
		if (interrupted_ || timeout_) {
			// interrupted or timed out threads need to be reset if they should be reused
			num_threads_in_wait_function_--;
			if (likely(data_->own_mutex))
				data_->mutex->unlock();
			return true;
		}
	}

	--data_->threads_left;
	try {
		passed_threads_->push_back_locked(Thread::current_thread());
	} catch (Exception &e) {
		// Cannot do anything more useful :-/
		// to stay fully compatible with Barrier we do *not* re-throw
		e.print_trace();
	}

	bool local_timeout = false;

	//Am I the last thread the interruptable  barrier is waiting for? Then I can wake the others up.
	bool waker = (data_->threads_left == 0);

	while (data_->threads_left && !interrupted_ && !timeout_ && !local_timeout) {
		//Here, the threads are waiting for the barrier
		//pthread_cond_timedwait releases data_->mutex if it is not external
		local_timeout = !data_->waitcond->reltimed_wait(timeout_sec, timeout_nanosec);
		//before continuing, pthread_cond_timedwait locks data_->mutex again if it is not external
	}

	if (local_timeout) {
		//set timeout flag of the interruptable barrier so the other threads can continue
		timeout_ = true;
	}

	if (interrupted_) {
		if (likely(data_->own_mutex))
			data_->mutex->unlock();
		throw InterruptedException("InterruptibleBarrier forcefully interrupted, only "
		                           "%u of %u threads reached the barrier",
		                           _count - data_->threads_left,
		                           _count);
	}

	if (waker) {
		//all threads of this barrier have to synchronize at the standard Barrier
		wait_at_barrier_ = true;
	}

	if (waker || local_timeout) {
		//the other threads can stop waiting in th while-loop
		data_->waitcond->wake_all();
	}

	if (likely(data_->own_mutex))
		data_->mutex->unlock();

	if (wait_at_barrier_) {
		//hard synchronization
		Barrier::wait();
	}

	if (likely(data_->own_mutex))
		data_->mutex->lock();
	//increment is not threadsafe
	num_threads_in_wait_function_--;
	if (likely(data_->own_mutex))
		data_->mutex->unlock();

	return !timeout_;
}

/** Checks if there are no more threads in the wait() function.
 * This method is used to prevent the destruction of the barrier
 * while there are threads in wait().
 * @return true, if no thread currently is in wait()
 */
bool
InterruptibleBarrier::no_threads_in_wait()
{
	if (likely(data_->own_mutex))
		data_->mutex->lock();
	bool res = num_threads_in_wait_function_ == 0;
	if (likely(data_->own_mutex))
		data_->mutex->unlock();

	return res;
}

} // end namespace fawkes
