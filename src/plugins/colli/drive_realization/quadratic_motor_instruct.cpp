
/***************************************************************************
 *  quadratic_motor_instruct.cpp - Motor instructor with quadratic approximation
 *
 *  Created: Fri Oct 18 15:16:23 2013
 *  Copyright  2002  Stefan Jacobs
 *             2013  Bahram Maleki-Fard
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

#include "quadratic_motor_instruct.h"

#include <utils/math/common.h>

#include <string>

namespace fawkes {

using namespace std;

/** @class QuadraticMotorInstruct <plugins/colli/drive_realization/quadratic_motor_instruct.h>
 * This module is a class for validity checks of drive
 * commands and sets those things with respect to the physical
 * borders of the robot.
 * For this purpose the two functions CalculateRotation and
 * CalculateTranslation are implemented quadratically ;-)
 */

/** Constructor.
 * @param motor The MotorInterface with all the motor information
 * @param frequency The frequency of the colli (should become deprecated!)
 * @param logger The fawkes logger
 * @param config The fawkes configuration
 */
QuadraticMotorInstruct::QuadraticMotorInstruct(MotorInterface *motor,
                                               float           frequency,
                                               Logger *        logger,
                                               Configuration * config)
: BaseMotorInstruct(motor, frequency, logger, config)
{
	logger_->log_debug("QuadraticMotorInstruct", "(Constructor): Entering");
	logger_->log_debug("QuadraticMotorInstruct", "(Constructor): Exiting");
}

/** Destructor. */
QuadraticMotorInstruct::~QuadraticMotorInstruct()
{
	logger_->log_debug("QuadraticMotorInstruct", "(Destructor): Entering");
	logger_->log_debug("QuadraticMotorInstruct", "(Destructor): Exiting");
}

/** Implementation of Calculate Translation Function.
 * These are dangerous! Take care while modifying. Only a minus sign too few
 *   or too much may result in non predictable motor behaviour!!!!
 * THIS FUNCTION IS THE LAST BORDER TO THE MOTOR, TAKE CARE AND PAY ATTENTION!!!
 */
float
QuadraticMotorInstruct::calculate_translation(float current, float desired, float time_factor)
{
	float exec_trans = 0.0;

	if (desired < current) {
		if (current > 0.0) {
			// decrease forward speed
			exec_trans = current - trans_dec_ - ((sqr(fabs(current) + 1.0) * trans_dec_) / 8.0);
			exec_trans = max(exec_trans, desired);

		} else if (current < 0.0) {
			// increase backward speed
			exec_trans = current - trans_acc_ - ((sqr(fabs(current) + 1.0) * trans_acc_) / 8.0);
			exec_trans = max(exec_trans, desired);

		} else {
			// current == 0;
			exec_trans = max(-trans_acc_, desired);
		}

	} else if (desired > current) {
		if (current > 0.0) {
			// increase forward speed
			exec_trans = current + trans_acc_ + ((sqr(fabs(current) + 1.0) * trans_acc_) / 8.0);
			exec_trans = min(exec_trans, desired);

		} else if (current < 0.0) {
			// decrease backward speed
			exec_trans = current + trans_dec_ + ((sqr(fabs(current) + 1.0) * trans_dec_) / 8.0);
			exec_trans = min(exec_trans, desired);

		} else {
			// current == 0
			exec_trans = min(trans_acc_, desired);
		}

	} else {
		// nothing to change!!!
		exec_trans = desired;
	}

	return exec_trans * time_factor;
}

/** Implementation of Calculate Rotation Function.
 * These are dangerous! Take care while modifying. Only a minus sign too few
 *   or too much may result in non predictable motor behaviour!!!!
 * THIS FUNCTION IS THE LAST BORDER TO THE MOTOR, TAKE CARE AND PAY ATTENTION!!!
 */
float
QuadraticMotorInstruct::calculate_rotation(float current, float desired, float time_factor)
{
	float exec_rot = 0.0;

	if (desired < current) {
		if (current > 0.0) {
			// decrease right rot
			exec_rot = current - rot_dec_ - ((sqr(fabs(current) + 1.0) * rot_dec_) / 8.0);
			exec_rot = max(exec_rot, desired);

		} else if (current < 0.0) {
			// increase left rot
			exec_rot = current - rot_acc_ - ((sqr(fabs(current) + 1.0) * rot_acc_) / 8.0);
			exec_rot = max(exec_rot, desired);

		} else {
			// current == 0;
			exec_rot = max(-rot_acc_, desired);
		}

	} else if (desired > current) {
		if (current > 0.0) {
			// increase right rot
			exec_rot = current + rot_acc_ + ((sqr(fabs(current) + 1.0) * rot_acc_) / 8.0);
			exec_rot = min(exec_rot, desired);

		} else if (current < 0.0) {
			// decrease left rot
			exec_rot = current + rot_dec_ + ((sqr(fabs(current) + 1.0) * rot_dec_) / 8.0);
			exec_rot = min(exec_rot, desired);

		} else {
			// current == 0
			exec_rot = min(rot_acc_, desired);
		}

	} else {
		// nothing to change!!!
		exec_rot = desired;
	}

	return exec_rot * time_factor;
}

} // namespace fawkes
