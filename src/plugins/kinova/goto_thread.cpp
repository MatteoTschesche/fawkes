
/***************************************************************************
 *  goto_thread.cpp - Kinova plugin Jaco movement thread
 *
 *  Created: Thu Jun 20 15:04:20 2013
 *  Copyright  2013  Bahram Maleki-Fard
 *
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the doc directory.
 */

#include "goto_thread.h"
#include "openrave_base_thread.h"
#include "arm.h"

#include <interfaces/JacoInterface.h>
#include <utils/math/angle.h>
#include <core/threading/mutex.h>

#include <unistd.h>

using namespace fawkes;

/** @class KinovaGotoThread "goto_thread.h"
 * Jaco Arm movement thread.
 * This thread handles the movement of the arm.
 *
 * @author Bahram Maleki-Fard
 */

/** Constructor.
 * @param thread_name thread name
 */
KinovaGotoThread::KinovaGotoThread(const char name[])
  : Thread(name, Thread::OPMODE_CONTINUOUS)
{
  __arm = NULL;
  __final_mutex = NULL;

  __final = true;

  __wait_status_check = 0; //wait loops to check for jaco_retract_mode_t again
}


/** Destructor. */
KinovaGotoThread::~KinovaGotoThread()
{
}

void
KinovaGotoThread::init()
{
  __final_mutex = new Mutex();
}

void
KinovaGotoThread::finalize()
{
  delete __final_mutex;
  __final_mutex = NULL;
}

bool
KinovaGotoThread::final()
{
  // check if all targets have been processed
  __arm->target_mutex->lock();
  bool final = __arm->target_queue->empty();
  __arm->target_mutex->unlock();
  if( !final )
    return false; // still targets in queue

  // queue is empty. Now check if any movement has startet (__final would be false then)
  __final_mutex->lock();
  final = __final;
  __final_mutex->unlock();
  if( final )
    return true; // no movement

  // There was some movement initiated. Check if it has finished
  check_final();
  __final_mutex->lock();
  final = __final;
  __final_mutex->unlock();

  return final;
}

void
KinovaGotoThread::register_arm(fawkes::jaco_arm_t *arm)
{
  __arm = arm;
}

void
KinovaGotoThread::unregister_arm()
{
  __arm = NULL;
}

void
KinovaGotoThread::set_target(float x, float y, float z,
                             float e1, float e2, float e3,
                             float f1, float f2, float f3)
{
  RefPtr<jaco_target_t> target(new jaco_target_t());
  target->type = TARGET_CARTESIAN;

  target->pos.push_back(x);
  target->pos.push_back(y);
  target->pos.push_back(z);
  target->pos.push_back(e1);
  target->pos.push_back(e2);
  target->pos.push_back(e3);

  if( f1 > 0.f && f2 > 0.f && f3 > 0.f ) {
    target->fingers.push_back(f1);
    target->fingers.push_back(f2);
    target->fingers.push_back(f3);
  }
  __arm->target_mutex->lock();
  __arm->target_queue->push_back(target);
  __arm->target_mutex->unlock();
}

void
KinovaGotoThread::set_target_ang(float j1, float j2, float j3,
                                 float j4, float j5, float j6,
                                 float f1, float f2, float f3)
{
  RefPtr<jaco_target_t> target(new jaco_target_t());
  target->type = TARGET_ANGULAR;

  target->pos.push_back(j1);
  target->pos.push_back(j2);
  target->pos.push_back(j3);
  target->pos.push_back(j4);
  target->pos.push_back(j5);
  target->pos.push_back(j6);

  if( f1 > 0.f && f2 > 0.f && f3 > 0.f ) {
    target->fingers.push_back(f1);
    target->fingers.push_back(f2);
    target->fingers.push_back(f3);
  }
  __arm->target_mutex->lock();
  __arm->target_queue->push_back(target);
  __arm->target_mutex->unlock();
}

void
KinovaGotoThread::pos_ready()
{
  RefPtr<jaco_target_t> target(new jaco_target_t());
  target->type = TARGET_READY;
  __arm->target_mutex->lock();
  __arm->target_queue->push_back(target);
  __arm->target_mutex->unlock();
}

void
KinovaGotoThread::pos_retract()
{
  RefPtr<jaco_target_t> target(new jaco_target_t());
  target->type = TARGET_RETRACT;
  __arm->target_mutex->lock();
  __arm->target_queue->push_back(target);
  __arm->target_mutex->unlock();
}


void
KinovaGotoThread::move_gripper(float f1, float f2 ,float f3)
{
  RefPtr<jaco_target_t> target(new jaco_target_t());
  target->type = TARGET_GRIPPER;

  target->fingers.push_back(f1);
  target->fingers.push_back(f2);
  target->fingers.push_back(f3);

  __arm->target_mutex->lock();
  __arm->target_queue->push_back(target);
  __arm->target_mutex->unlock();
}

/** Stops the current movement.
 * This also stops any currently enqueued motion.
 */
void
KinovaGotoThread::stop()
{
  try {
    __arm->arm->stop();

    __arm->target_mutex->lock();
    __arm->target_queue->clear();
    __arm->target_mutex->unlock();

    __final_mutex->lock();
    __final = true;
    __final_mutex->unlock();

  } catch( Exception &e ) {
    logger->log_warn(name(), "Error sending stop command to arm. Ex:%s", e.what());
  }
}

void
KinovaGotoThread::check_final()
{
  bool check_fingers = false;
  bool final = true;

  //logger->log_debug(name(), "check final");
  switch( __target->type ) {

    case TARGET_READY:
    case TARGET_RETRACT:
      if( __wait_status_check == 0 ) {
        //logger->log_debug(name(), "check final for TARGET_MODE");
        __final_mutex->lock();
        __final = __arm->arm->final();
        __final_mutex->unlock();
      } else if( __wait_status_check >= 10 ) {
          __wait_status_check = 0;
      } else {
          ++__wait_status_check;
      }
      break;

/*
    default: //TARGET_ANGULAR, TARGET_CARTESIAN
      __final = __arm->arm->final();
//*/
//*
    case TARGET_TRAJEC:
    case TARGET_ANGULAR:
      //logger->log_debug(name(), "check final for TARGET ANGULAR");
      //final = __arm->arm->final();
      for( unsigned int i=0; i<6; ++i ) {
        final &= (std::abs(normalize_mirror_rad(deg2rad(__target->pos.at(i) - __arm->iface->joints(i)))) < 0.01);
      }
      __final_mutex->lock();
      __final = final;
      __final_mutex->unlock();
      check_fingers = true;
      break;

    default: //TARGET_CARTESIAN
      //logger->log_debug(name(), "check final for TARGET CARTESIAN");
      //final = __arm->arm->final();
      final &= (std::abs(angle_distance(__target->pos.at(0) , __arm->iface->x())) < 0.01);
      final &= (std::abs(angle_distance(__target->pos.at(1) , __arm->iface->y())) < 0.01);
      final &= (std::abs(angle_distance(__target->pos.at(2) , __arm->iface->z())) < 0.01);
      final &= (std::abs(angle_distance(__target->pos.at(3) , __arm->iface->euler1())) < 0.1);
      final &= (std::abs(angle_distance(__target->pos.at(4) , __arm->iface->euler2())) < 0.1);
      final &= (std::abs(angle_distance(__target->pos.at(5) , __arm->iface->euler3())) < 0.1);
      __final_mutex->lock();
      __final = final;
      __final_mutex->unlock();

      check_fingers = true;
      break;
//*/

  }

  //logger->log_debug(name(), "check final: %u", __final);

  if( check_fingers && __final ) {
    //logger->log_debug(name(), "check fingeres for final");

    // also check fingeres
    if( __finger_last[0] == __arm->iface->finger1() &&
        __finger_last[1] == __arm->iface->finger2() &&
        __finger_last[2] == __arm->iface->finger3() ) {
      __finger_last[3] += 1;
    } else {
      __finger_last[0] = __arm->iface->finger1();
      __finger_last[1] = __arm->iface->finger2();
      __finger_last[2] = __arm->iface->finger3();
      __finger_last[3] = 0; // counter
    }
    __final_mutex->lock();
    __final &= __finger_last[3] > 10;
    __final_mutex->unlock();
  }
}

void
KinovaGotoThread::loop()
{
  if(__arm == NULL) {
    usleep(30e3);
    return;
  }

  if( __final) {
   // Current target has been processed. Unref, if still refed
    if(__target) {
      __target.clear();
    }

    // Check for new targets
    __arm->target_mutex->lock();
    if( !__arm->target_queue->empty() ) {
      // get RefPtr to first target in queue
      __target = __arm->target_queue->front();
    }
    __arm->target_mutex->unlock();

    if( __target ) {
      if( __target->type == TARGET_TRAJEC ) {
        logger->log_debug(name(), "next target is a trajectory...");

        if( __target->trajec_state == TRAJEC_READY ) {
          logger->log_debug(name(), "... and ready! processing now.");
          // update trajectory state
          __arm->target_mutex->lock();
          __target->trajec_state = TRAJEC_EXECUTING;
          __arm->target_mutex->unlock();

          // process trajectory only if it actually "exists"
          if( !__target->trajec->empty() ) {
            // first let the openrave_thread show the trajectory in the viewer
            __arm->openrave_thread->plot_first();

            // then execute the trajectory
            _exec_trajec(*(__target->trajec));
          }

          // trajectory has been processed. remove that target from queue.
          // This will automatically delete the trajectory as well as soon
          // as we leave this block (thanks to refptr)
          __arm->target_mutex->lock();
          __arm->target_queue->pop_front();
          __arm->target_mutex->unlock();

        } else if (__target->trajec_state == TRAJEC_PLANNING_ERROR ) {
          logger->log_debug(name(), "... but the trajectory could not be planned. Abort!");
          // stop the current and remaining queue, with appropriate error_code. This also sets "final" to true.
          stop();
          __arm->iface->set_error_code( JacoInterface::ERROR_PLANNING );

        } else {
          logger->log_debug(name(), "... but not ready yet!");
          usleep(30e3);
        }

      } else {
        // "regular" target
        logger->log_debug(name(), "Process new target. using current finger positions");
        _goto_target();

        __arm->target_mutex->lock();
        __arm->target_queue->pop_front();
        __arm->target_mutex->unlock();
      }

    } else {
      //no new target in queue
      usleep(30e3);
    }

  } else {
    usleep(30e3);
  }

//  __arm->iface->set_final(__final);
}



void
KinovaGotoThread::_goto_target()
{

  __finger_last[0] = __arm->iface->finger1();
  __finger_last[1] = __arm->iface->finger2();
  __finger_last[2] = __arm->iface->finger3();
  __finger_last[3] = 0; // counter

  __final = false;

  // process new target
  try {
    __arm->arm->stop(); // stop old movement, if there was any
    //__arm->arm->start_api_ctrl();

   if( __target->type == TARGET_GRIPPER ) {
     // only fingers moving. use current joint values for that
     // we do this here and not in "move_gripper()" because we enqueue values. This ensures
     // that we move the gripper with the current joint values, not with the ones we had
     // when the target was enqueued!
     __target->pos.clear(); // just in case; should be empty anyway
     __target->pos.push_back(__arm->iface->joints(0));
     __target->pos.push_back(__arm->iface->joints(1));
     __target->pos.push_back(__arm->iface->joints(2));
     __target->pos.push_back(__arm->iface->joints(3));
     __target->pos.push_back(__arm->iface->joints(4));
     __target->pos.push_back(__arm->iface->joints(5));
     __target->type = TARGET_ANGULAR;
   }

    switch( __target->type ) {
      case TARGET_ANGULAR:
        logger->log_debug(name(), "target_type: TARGET_ANGULAR");
        if( __target->fingers.empty() ) {
          // have no finger values. use current ones
          __target->fingers.push_back(__arm->iface->finger1());
          __target->fingers.push_back(__arm->iface->finger2());
          __target->fingers.push_back(__arm->iface->finger3());
        }
        __arm->arm->goto_joints(__target->pos, __target->fingers);
        break;

      case TARGET_READY:
        logger->log_debug(name(), "loop: target_type: TARGET_READY");
        __wait_status_check = 0;
        __arm->arm->goto_ready();
        break;

      case TARGET_RETRACT:
        logger->log_debug(name(), "target_type: TARGET_RETRACT");
        __wait_status_check = 0;
        __arm->arm->goto_retract();
        break;

      default: //TARGET_CARTESIAN
        logger->log_debug(name(), "target_type: TARGET_CARTESIAN");
        if( __target->fingers.empty() ) {
          // have no finger values. use current ones
          __target->fingers.push_back(__arm->iface->finger1());
          __target->fingers.push_back(__arm->iface->finger2());
          __target->fingers.push_back(__arm->iface->finger3());
        }
        __arm->arm->goto_coords(__target->pos, __target->fingers);
        break;
    }

  } catch( Exception &e ) {
    logger->log_warn(name(), "Error sending command to arm. Ex:%s", e.what_no_backtrace());
  }
}

void
KinovaGotoThread::_exec_trajec(jaco_trajec_t* trajec)
{
  if( __target->fingers.empty() ) {
    // have no finger values. use current ones
    __target->fingers.push_back(__arm->iface->finger1());
    __target->fingers.push_back(__arm->iface->finger2());
    __target->fingers.push_back(__arm->iface->finger3());
  }

  try {
     // stop old movement
    __arm->arm->stop();

    logger->log_debug(name(), "exec traj: send traj commands...");
    // execute the trajectory
    for( unsigned int i=0; i<trajec->size(); ++i ) {
      __arm->arm->goto_joints(trajec->at(i), __target->fingers);
      usleep(10e3);
    }
    logger->log_debug(name(), "exec traj: ... DONE");

  } catch( Exception &e ) {
    logger->log_warn(name(), "Error executing trajectory. Ex:%s", e.what_no_backtrace());
  }
}
