
/***************************************************************************
 *  mainloop.h - Fawkes MainLoopAspect initializer/finalizer
 *
 *  Created: Wed Nov 24 00:43:26 2010
 *  Copyright  2006-2010  Tim Niemueller [www.niemueller.de]
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

#ifndef _ASPECT_INIFINS_MAINLOOP_H_
#define _ASPECT_INIFINS_MAINLOOP_H_

#include <aspect/inifins/inifin.h>
#include <aspect/mainloop.h>
#include <core/threading/thread_notification_listener.h>
#include <utils/constraints/unique.h>

namespace fawkes {

class MainLoopEmployer;
class BlockedTimingExecutor;

class MainLoopAspectIniFin : public AspectIniFin, public ThreadNotificationListener
{
public:
	MainLoopAspectIniFin(MainLoopEmployer *employer, BlockedTimingExecutor *btexec);

	virtual void init(Thread *thread);
	virtual void finalize(Thread *thread);

	virtual bool thread_started(Thread *thread) noexcept;
	virtual bool thread_init_failed(Thread *thread) noexcept;

private:
	MainLoopEmployer *                   employer_;
	BlockedTimingExecutor *              btexec_;
	UniquenessConstraint<MainLoopAspect> mainloop_uc_;
};

} // end namespace fawkes

#endif
