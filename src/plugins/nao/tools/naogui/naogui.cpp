
/***************************************************************************
 *  naogui.cpp - Nao GUI
 *
 *  Created: Mon Apr 14 12:54:19 2008
 *  Copyright  2008-2011  Tim Niemueller [www.niemueller.de]
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

#include "naogui.h"

#include <utils/system/argparser.h>
#include <blackboard/remote.h>
#include <interfaces/NaoJointPositionInterface.h>
#include <interfaces/NaoJointStiffnessInterface.h>
#include <interfaces/NaoSensorInterface.h>
#include <interfaces/HumanoidMotionInterface.h>
#include <interfaces/NavigatorInterface.h>
#include <netcomm/fawkes/client.h>

#include <gui_utils/service_chooser_dialog.h>
#include <gui_utils/interface_dispatcher.h>

#include <cstring>
#include <string>
#include <sstream>
#include <iomanip>
#include <sstream>
#include <gtkmm/clipboard.h>

using namespace fawkes;


/** @class NaoGuiGtkWindow "naogui.h"
 * Nao GUI main window.
 * The Nao GUI provides access to the most basic servo, sensor and motion
 * data and commands.
 * @author Tim Niemueller
 */

/** Constructor.
 * @param cobject C base object
 * @param builder Gtk builder to get widgets from
 */
NaoGuiGtkWindow::NaoGuiGtkWindow(BaseObjectType* cobject,
				 const Glib::RefPtr<Gtk::Builder> &builder)
  : Gtk::Window(cobject)
{
  servo_time = 500;
  bb = NULL;
  jointpos_if = NULL;
  jointstiff_if = NULL;
  sensor_if = NULL;
  nao_navi_if = NULL;
  hummot_naoqi_if = NULL;
  hummot_fawkes_if = NULL;
  servo_enabled = false;

  update_cycle = 0;

  builder->get_widget("frm_servos", frm_servos);
  builder->get_widget("frm_sensors", frm_sensors);
  builder->get_widget("frm_ultrasonic", frm_ultrasonic);

  builder->get_widget("lab_HeadYaw", lab_HeadYaw);
  builder->get_widget("lab_HeadPitch", lab_HeadPitch);
  builder->get_widget("lab_RShoulderPitch", lab_RShoulderPitch);
  builder->get_widget("lab_RShoulderRoll", lab_RShoulderRoll);
  builder->get_widget("lab_LShoulderPitch", lab_LShoulderPitch);
  builder->get_widget("lab_LShoulderRoll", lab_LShoulderRoll);
  builder->get_widget("lab_LElbowYaw", lab_LElbowYaw);
  builder->get_widget("lab_LElbowRoll", lab_LElbowRoll);
  builder->get_widget("lab_LWristYaw", lab_LWristYaw);
  builder->get_widget("lab_LHand", lab_LHand);
  builder->get_widget("lab_RElbowYaw", lab_RElbowYaw);
  builder->get_widget("lab_RElbowRoll", lab_RElbowRoll);
  builder->get_widget("lab_RWristYaw", lab_RWristYaw);
  builder->get_widget("lab_RHand", lab_RHand);
  builder->get_widget("lab_RHipYawPitch", lab_RHipYawPitch);
  builder->get_widget("lab_RHipPitch", lab_RHipPitch);
  builder->get_widget("lab_RHipRoll", lab_RHipRoll);
  builder->get_widget("lab_RKneePitch", lab_RKneePitch);
  builder->get_widget("lab_RAnklePitch", lab_RAnklePitch);
  builder->get_widget("lab_RAnkleRoll", lab_RAnkleRoll);
  builder->get_widget("lab_LHipYawPitch", lab_LHipYawPitch);
  builder->get_widget("lab_LHipPitch", lab_LHipPitch);
  builder->get_widget("lab_LHipRoll", lab_LHipRoll);
  builder->get_widget("lab_LKneePitch", lab_LKneePitch);
  builder->get_widget("lab_LAnklePitch", lab_LAnklePitch);
  builder->get_widget("lab_LAnkleRoll", lab_LAnkleRoll);
  builder->get_widget("lab_stiff_HeadYaw", lab_stiff_HeadYaw);
  builder->get_widget("lab_stiff_HeadPitch", lab_stiff_HeadPitch);
  builder->get_widget("lab_stiff_RShoulderPitch", lab_stiff_RShoulderPitch);
  builder->get_widget("lab_stiff_RShoulderRoll", lab_stiff_RShoulderRoll);
  builder->get_widget("lab_stiff_LShoulderPitch", lab_stiff_LShoulderPitch);
  builder->get_widget("lab_stiff_LShoulderRoll", lab_stiff_LShoulderRoll);
  builder->get_widget("lab_stiff_LElbowYaw", lab_stiff_LElbowYaw);
  builder->get_widget("lab_stiff_LElbowRoll", lab_stiff_LElbowRoll);
  builder->get_widget("lab_stiff_LWristYaw", lab_stiff_LWristYaw);
  builder->get_widget("lab_stiff_LHand", lab_stiff_LHand);
  builder->get_widget("lab_stiff_RElbowYaw", lab_stiff_RElbowYaw);
  builder->get_widget("lab_stiff_RElbowRoll", lab_stiff_RElbowRoll);
  builder->get_widget("lab_stiff_RWristYaw", lab_stiff_RWristYaw);
  builder->get_widget("lab_stiff_RHand", lab_stiff_RHand);
  builder->get_widget("lab_stiff_RHipYawPitch", lab_stiff_RHipYawPitch);
  builder->get_widget("lab_stiff_RHipPitch", lab_stiff_RHipPitch);
  builder->get_widget("lab_stiff_RHipRoll", lab_stiff_RHipRoll);
  builder->get_widget("lab_stiff_RKneePitch", lab_stiff_RKneePitch);
  builder->get_widget("lab_stiff_RAnklePitch", lab_stiff_RAnklePitch);
  builder->get_widget("lab_stiff_RAnkleRoll", lab_stiff_RAnkleRoll);
  builder->get_widget("lab_stiff_LHipYawPitch", lab_stiff_LHipYawPitch);
  builder->get_widget("lab_stiff_LHipPitch", lab_stiff_LHipPitch);
  builder->get_widget("lab_stiff_LHipRoll", lab_stiff_LHipRoll);
  builder->get_widget("lab_stiff_LKneePitch", lab_stiff_LKneePitch);
  builder->get_widget("lab_stiff_LAnklePitch", lab_stiff_LAnklePitch);
  builder->get_widget("lab_stiff_LAnkleRoll", lab_stiff_LAnkleRoll);
  builder->get_widget("hsc_HeadYaw", hsc_HeadYaw);
  builder->get_widget("hsc_HeadPitch", hsc_HeadPitch);
  builder->get_widget("hsc_RShoulderPitch", hsc_RShoulderPitch);
  builder->get_widget("hsc_RShoulderRoll", hsc_RShoulderRoll);
  builder->get_widget("hsc_RElbowYaw", hsc_RElbowYaw);
  builder->get_widget("hsc_RElbowRoll", hsc_RElbowRoll);
  builder->get_widget("hsc_RWristYaw", hsc_RWristYaw);
  builder->get_widget("hsc_RHand", hsc_RHand);
  builder->get_widget("hsc_LShoulderPitch", hsc_LShoulderPitch);
  builder->get_widget("hsc_LShoulderRoll", hsc_LShoulderRoll);
  builder->get_widget("hsc_LElbowYaw", hsc_LElbowYaw);
  builder->get_widget("hsc_LElbowRoll", hsc_LElbowRoll);
  builder->get_widget("hsc_LWristYaw", hsc_LWristYaw);
  builder->get_widget("hsc_LHand", hsc_LHand);
  builder->get_widget("hsc_RHipYawPitch", hsc_RHipYawPitch);
  builder->get_widget("hsc_RHipPitch", hsc_RHipPitch);
  builder->get_widget("hsc_RHipRoll", hsc_RHipRoll);
  builder->get_widget("hsc_RKneePitch", hsc_RKneePitch);
  builder->get_widget("hsc_RAnklePitch", hsc_RAnklePitch);
  builder->get_widget("hsc_RAnkleRoll", hsc_RAnkleRoll);
  builder->get_widget("hsc_LHipYawPitch", hsc_LHipYawPitch);
  builder->get_widget("hsc_LHipPitch", hsc_LHipPitch);
  builder->get_widget("hsc_LHipRoll", hsc_LHipRoll);
  builder->get_widget("hsc_LKneePitch", hsc_LKneePitch);
  builder->get_widget("hsc_LAnklePitch", hsc_LAnklePitch);
  builder->get_widget("hsc_LAnkleRoll", hsc_LAnkleRoll);
  builder->get_widget("hsc_time", hsc_time);
  builder->get_widget("lab_time", lab_time);
  builder->get_widget("tb_connection", tb_connection);
  builder->get_widget("tb_stiffness", tb_stiffness);
  builder->get_widget("tb_control", tb_control);
  builder->get_widget("tb_getup", tb_getup);
  builder->get_widget("tb_parkpos", tb_parkpos);
  builder->get_widget("tb_zeroall", tb_zeroall);
  builder->get_widget("tb_exit", tb_exit);
  builder->get_widget("lab_l_fsr_fl", lab_l_fsr_fl);
  builder->get_widget("lab_l_fsr_fr", lab_l_fsr_fr);
  builder->get_widget("lab_l_fsr_rl", lab_l_fsr_rl);
  builder->get_widget("lab_l_fsr_rr", lab_l_fsr_rr);
  builder->get_widget("lab_r_fsr_fl", lab_r_fsr_fl);
  builder->get_widget("lab_r_fsr_fr", lab_r_fsr_fr);
  builder->get_widget("lab_r_fsr_rl", lab_r_fsr_rl);
  builder->get_widget("lab_r_fsr_rr", lab_r_fsr_rr);
  builder->get_widget("lab_l_cop", lab_l_cop);
  builder->get_widget("lab_r_cop", lab_r_cop);
  builder->get_widget("lab_l_total_weight", lab_l_total_weight);
  builder->get_widget("lab_r_total_weight", lab_r_total_weight);
  builder->get_widget("lab_chest_button", lab_chest_button);
  builder->get_widget("lab_touch_front", lab_touch_front);
  builder->get_widget("lab_touch_middle", lab_touch_middle);
  builder->get_widget("lab_touch_rear", lab_touch_rear);
  builder->get_widget("lab_l_bumper_l", lab_l_bumper_l);
  builder->get_widget("lab_l_bumper_r", lab_l_bumper_r);
  builder->get_widget("lab_r_bumper_l", lab_r_bumper_l);
  builder->get_widget("lab_r_bumper_r", lab_r_bumper_r);
  builder->get_widget("lab_accel_x", lab_accel_x);
  builder->get_widget("lab_accel_y", lab_accel_y);
  builder->get_widget("lab_accel_z", lab_accel_z);
  builder->get_widget("lab_gyro_x", lab_gyro_x);
  builder->get_widget("lab_gyro_y", lab_gyro_y);
  builder->get_widget("lab_gyro_ref", lab_gyro_ref);
  builder->get_widget("lab_angles_xy", lab_angles_xy);
  builder->get_widget("lab_ultrasonic_distance", lab_ultrasonic_distance);
  builder->get_widget("lab_ultrasonic_direction", lab_ultrasonic_direction);
  builder->get_widget("lab_battery_charge", lab_battery_charge);
  builder->get_widget("but_sv_copy", but_sv_copy);
  builder->get_widget("cmb_us_direction", cmb_us_direction);
  builder->get_widget("but_us_emit", but_us_emit);
  builder->get_widget("but_us_auto", but_us_auto);
  builder->get_widget("but_stop", but_stop);
  builder->get_widget("but_ws_exec", but_ws_exec);
  builder->get_widget("ent_ws_distance", ent_ws_distance);
  builder->get_widget("but_wsw_exec", but_wsw_exec);
  builder->get_widget("ent_wsw_distance", ent_wsw_distance);
  builder->get_widget("but_wa_exec", but_wa_exec);
  builder->get_widget("ent_wa_angle", ent_wa_angle);
  builder->get_widget("ent_wa_radius", ent_wa_radius);
  builder->get_widget("but_turn_exec", but_turn_exec);
  builder->get_widget("cmb_kick_leg", cmb_kick_leg);
  builder->get_widget("ent_kick_strength", ent_kick_strength);
  builder->get_widget("but_kick_exec", but_kick_exec);
  builder->get_widget("ent_turn_angle", ent_turn_angle);
  builder->get_widget("rad_motion_fawkes", rad_motion_fawkes);
  builder->get_widget("rad_motion_naoqi", rad_motion_naoqi);
  builder->get_widget("ent_walkvel_x", ent_walkvel_x);
  builder->get_widget("ent_walkvel_y", ent_walkvel_y);
  builder->get_widget("ent_walkvel_theta", ent_walkvel_theta);
  builder->get_widget("ent_walkvel_speed", ent_walkvel_speed);
  builder->get_widget("but_walkvel_exec", but_walkvel_exec);

  builder->get_widget("but_stiffness_read", but_stiffness_read);
  builder->get_widget("but_stiffness_write", but_stiffness_write);
  builder->get_widget("chb_stiffness_global", chb_stiffness_global);
  builder->get_widget("spb_stiffness_global", spb_stiffness_global);
  builder->get_widget("spb_HeadYaw", spb_HeadYaw);
  builder->get_widget("spb_HeadPitch", spb_HeadPitch);
  builder->get_widget("spb_RShoulderPitch", spb_RShoulderPitch);
  builder->get_widget("spb_RShoulderRoll", spb_RShoulderRoll);
  builder->get_widget("spb_RElbowYaw", spb_RElbowYaw);
  builder->get_widget("spb_RElbowRoll", spb_RElbowRoll);
  builder->get_widget("spb_RWristYaw", spb_RWristYaw);
  builder->get_widget("spb_RHand", spb_RHand);
  builder->get_widget("spb_LShoulderPitch", spb_LShoulderPitch);
  builder->get_widget("spb_LShoulderRoll", spb_LShoulderRoll);
  builder->get_widget("spb_LElbowYaw", spb_LElbowYaw);
  builder->get_widget("spb_LElbowRoll", spb_LElbowRoll);
  builder->get_widget("spb_LWristYaw", spb_LWristYaw);
  builder->get_widget("spb_LHand", spb_LHand);
  builder->get_widget("spb_RHipYawPitch", spb_RHipYawPitch);
  builder->get_widget("spb_RHipPitch", spb_RHipPitch);
  builder->get_widget("spb_RHipRoll", spb_RHipRoll);
  builder->get_widget("spb_RKneePitch", spb_RKneePitch);
  builder->get_widget("spb_RAnklePitch", spb_RAnklePitch);
  builder->get_widget("spb_RAnkleRoll", spb_RAnkleRoll);
  builder->get_widget("spb_LHipYawPitch", spb_LHipYawPitch);
  builder->get_widget("spb_LHipPitch", spb_LHipPitch);
  builder->get_widget("spb_LHipRoll", spb_LHipRoll);
  builder->get_widget("spb_LKneePitch", spb_LKneePitch);
  builder->get_widget("spb_LAnklePitch", spb_LAnklePitch);
  builder->get_widget("spb_LAnkleRoll", spb_LAnkleRoll);

  builder->get_widget("ent_nav_x", ent_nav_x);
  builder->get_widget("ent_nav_y", ent_nav_y);
  builder->get_widget("ent_nav_ori", ent_nav_ori);
  builder->get_widget("but_nav_exec", but_nav_exec);

  cmb_kick_leg->set_active(0);
  cmb_us_direction->set_active(0);
  frm_servos->set_sensitive(false);
  frm_sensors->set_sensitive(false);
  frm_ultrasonic->set_sensitive(false);

  hsc_HeadYaw->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_HeadYaw, lab_HeadYaw, NaoJointPositionInterface::SERVO_head_yaw));
  hsc_HeadPitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_HeadPitch, lab_HeadPitch, NaoJointPositionInterface::SERVO_head_pitch));
  hsc_RShoulderPitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RShoulderPitch, lab_RShoulderPitch, NaoJointPositionInterface::SERVO_r_shoulder_pitch));
  hsc_RShoulderRoll->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RShoulderRoll, lab_RShoulderRoll, NaoJointPositionInterface::SERVO_r_shoulder_roll));
  hsc_RElbowYaw->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RElbowYaw, lab_RElbowYaw, NaoJointPositionInterface::SERVO_r_elbow_yaw));
  hsc_RElbowRoll->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RElbowRoll, lab_RElbowRoll, NaoJointPositionInterface::SERVO_r_elbow_roll));
  hsc_RWristYaw->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RWristYaw, lab_RWristYaw, NaoJointPositionInterface::SERVO_r_wrist_yaw));
  hsc_RHand->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RHand, lab_RHand, NaoJointPositionInterface::SERVO_r_hand));
  hsc_LShoulderPitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LShoulderPitch, lab_LShoulderPitch, NaoJointPositionInterface::SERVO_l_shoulder_pitch));
  hsc_LShoulderRoll->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LShoulderRoll, lab_LShoulderRoll, NaoJointPositionInterface::SERVO_l_shoulder_roll));
  hsc_LElbowYaw->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LElbowYaw, lab_LElbowYaw, NaoJointPositionInterface::SERVO_l_elbow_yaw));
  hsc_LElbowRoll->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LElbowRoll, lab_LElbowRoll, NaoJointPositionInterface::SERVO_l_elbow_roll));
  hsc_LWristYaw->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LWristYaw, lab_LWristYaw, NaoJointPositionInterface::SERVO_r_wrist_yaw));
  hsc_LHand->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LHand, lab_LHand, NaoJointPositionInterface::SERVO_r_hand));
  hsc_RHipYawPitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RHipYawPitch, lab_RHipYawPitch, NaoJointPositionInterface::SERVO_r_hip_yaw_pitch));
  hsc_RHipPitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RHipPitch, lab_RHipPitch, NaoJointPositionInterface::SERVO_r_hip_pitch));
  hsc_RHipRoll->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RHipRoll, lab_RHipRoll, NaoJointPositionInterface::SERVO_r_hip_roll));
  hsc_RKneePitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RKneePitch, lab_RKneePitch, NaoJointPositionInterface::SERVO_r_knee_pitch));
  hsc_RAnklePitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RAnklePitch, lab_RAnklePitch, NaoJointPositionInterface::SERVO_r_ankle_pitch));
  hsc_RAnkleRoll->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_RAnkleRoll, lab_RAnkleRoll, NaoJointPositionInterface::SERVO_r_ankle_roll));
  hsc_LHipYawPitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LHipYawPitch, lab_LHipYawPitch, NaoJointPositionInterface::SERVO_l_hip_yaw_pitch));
  hsc_LHipPitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LHipPitch, lab_LHipPitch, NaoJointPositionInterface::SERVO_l_hip_pitch));
  hsc_LHipRoll->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LHipRoll, lab_LHipRoll, NaoJointPositionInterface::SERVO_l_hip_roll));
  hsc_LKneePitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LKneePitch, lab_LKneePitch, NaoJointPositionInterface::SERVO_l_knee_pitch));
  hsc_LAnklePitch->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LAnklePitch, lab_LAnklePitch, NaoJointPositionInterface::SERVO_l_ankle_pitch));
  hsc_LAnkleRoll->signal_value_changed().connect(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_slider_changed), hsc_LAnkleRoll, lab_LAnkleRoll, NaoJointPositionInterface::SERVO_l_ankle_roll));
  hsc_time->signal_value_changed().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_changed_time));
  tb_connection->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_connection_clicked));
  tb_stiffness->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_stiffness_clicked));
  tb_control->signal_toggled().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_control_toggled));
  tb_parkpos->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_goto_parkpos_clicked));
  tb_zeroall->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_goto_zero_all_clicked));
  tb_getup->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_get_up_clicked));
  tb_exit->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_exit_clicked));
  but_sv_copy->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_sv_copy_clicked));
  but_us_auto->signal_toggled().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_us_auto_toggled));
  but_us_emit->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_us_emit_clicked));
  but_walkvel_exec->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_walkvel_exec_clicked));
  but_ws_exec->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_ws_exec_clicked));
  but_stop->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_motion_stop_clicked));
  but_wsw_exec->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_wsw_exec_clicked));
  but_wa_exec->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_wa_exec_clicked));
  but_kick_exec->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_kick_exec_clicked));
  but_turn_exec->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_turn_exec_clicked));
  but_nav_exec->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_nav_exec_clicked));
  but_stiffness_read->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_stiffness_read_clicked));
  but_stiffness_write->signal_clicked().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_stiffness_write_clicked));
  chb_stiffness_global->signal_toggled().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_stiffness_global_toggled));

  connection_dispatcher.signal_connected().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_connect));
  connection_dispatcher.signal_disconnected().connect(sigc::mem_fun(*this, &NaoGuiGtkWindow::on_disconnect));

  on_control_toggled();
  init();
}


/** Destructor. */
NaoGuiGtkWindow::~NaoGuiGtkWindow()
{
  on_disconnect();
}

/**
 * Sets the default values (locale dependent)
 */
void
NaoGuiGtkWindow::init()
{
  ent_walkvel_x->set_text(convert_float2str(0.5f, 1));
  ent_walkvel_y->set_text(convert_float2str(0.f, 1));
  ent_walkvel_theta->set_text(convert_float2str(0.f, 1));
  ent_walkvel_speed->set_text(convert_float2str(0.5f, 1));

  ent_ws_distance->set_text(convert_float2str(0.2f, 1));

  ent_wsw_distance->set_text(convert_float2str(0.2f, 1));

  ent_wa_angle->set_text(convert_float2str(1.f, 1));
  ent_wa_radius->set_text(convert_float2str(0.3f, 1));

  ent_turn_angle->set_text(convert_float2str(1.f, 1));

  ent_nav_x->set_text(convert_float2str(0.f, 1));
  ent_nav_y->set_text(convert_float2str(0.f, 1));
  ent_nav_ori->set_text(convert_float2str(0.f, 1));
}

/** Update a servo value.
 * @param hsc horizontal scale for value
 * @param label label for value
 * @param value servo value
 */
void
NaoGuiGtkWindow::update_servo_value(Gtk::HScale *hsc, Gtk::Label *label, float value)
{
  float f = roundf(value * 100.);
  if ( ! tb_control->get_active() && (hsc->get_value() != f)) {
    hsc->set_value(f);
  }
  // this might seem more expensive than just setting it, but the really
  // expensive thing are screen updates, and they can be avoid much of the
  // time
  Glib::ustring fs = convert_float2str(value);
  if (label->get_text() != fs) {
    label->set_text(fs);
  }
}


/** Update sensor value.
 * @param label label for value
 * @param value sensor value
 * @param show_decimal true to show as decimal, false to show as integer
 */
void
NaoGuiGtkWindow::update_sensor_value(Gtk::Label *label, float value, bool show_decimal)
{
  Glib::ustring s = convert_float2str(value, show_decimal ? 2 : 0);
  if (label->get_text() != s) {
    label->set_text(s);
  }
}


/** Update an entry value.
 * @param ent entry field for the value
 * @param value the value to set
 * @param width the number of positions after decimal point
 */
void
NaoGuiGtkWindow::update_entry_value(Gtk::Entry *ent, float value, unsigned int width)
{
  if ( value != -123456 ) {
    ent->set_text(convert_float2str(value, width));
  } else {
    ent->set_text("");
  }
}


/** Update ultrasonic direction field.
 * @param direction direction value from interface field
 */
void
NaoGuiGtkWindow::update_ultrasonic_direction(float direction)
{
  if ( direction == NaoSensorInterface::USD_left_left ) {
    lab_ultrasonic_direction->set_text("l-l");
  } else if ( direction == NaoSensorInterface::USD_left_right ) {
    lab_ultrasonic_direction->set_text("l-r");
  } else if ( direction == NaoSensorInterface::USD_right_left ) {
    lab_ultrasonic_direction->set_text("r-l");
  } else if ( direction == NaoSensorInterface::USD_right_right ) {
    lab_ultrasonic_direction->set_text("r-r");
  }
}


/** Send servo message.
 * @param hsc scale from which to take the value from
 * @param servo the servo constant for the servo to set from the
 * NaoJointPositionInterface
 */
void
NaoGuiGtkWindow::send_servo_msg(Gtk::HScale *hsc, unsigned int servo)
{
  if ( jointpos_if && tb_control->get_active() ) {
    jointpos_if->read();

    if (servo == NaoJointPositionInterface::SERVO_head_pitch || servo == NaoJointPositionInterface::SERVO_head_yaw) {
      HumanoidMotionInterface::YawPitchHeadMessage *m
        = new HumanoidMotionInterface::YawPitchHeadMessage(hsc_HeadYaw->get_value() / 100.f, hsc_HeadPitch->get_value() / 100.f, servo_time / 1000.f);

      if ( rad_motion_fawkes->get_active() ) {
        hummot_fawkes_if->msgq_enqueue(m);
      } else {
        hummot_naoqi_if->msgq_enqueue(m);
      }
    }
    else {
      NaoJointPositionInterface::SetServoMessage *m
        = new NaoJointPositionInterface::SetServoMessage(servo,
							 hsc->get_value() / 100.f,
							 servo_time);

      jointpos_if->msgq_enqueue(m);
    }
  }
}


/** Event handler for slider changes.
 * @param hsc horizontal slider that caused the event
 * @param label label to set with the value
 * @param servo the servo constant for the servo to set from the
 * NaoJointPositionInterface
 */
void
NaoGuiGtkWindow::on_slider_changed(Gtk::HScale *hsc, Gtk::Label *label,
				   unsigned int servo)
{
  send_servo_msg(hsc, servo);
}


/** Time change event handler. */
void
NaoGuiGtkWindow::on_changed_time()
{
  char *tmp;
  if (asprintf(&tmp, "%d", (int)hsc_time->get_value()) != -1) {
    lab_time->set_text(tmp);
    free(tmp);
  }

  servo_time = (int)hsc_time->get_value();
}


/** Update joint position values.
 * Called whenever the NaoJointPositionInterface changes.
 * @param force true to force an update, false to update depending
 * on internal state
 */
void
NaoGuiGtkWindow::update_jointpos_values(bool force)
{
  if ( ! jointpos_if ) return;

  /*
  // we take only each 10th update to decrease ffnaogui CPU usage
  if (! force && (++update_cycle % 10 != 0)) {
    return;
  }
  update_cycle = 0;
  */

  try {
    jointpos_if->read();
    update_servo_value(hsc_HeadYaw, lab_HeadYaw, jointpos_if->head_yaw());
    update_servo_value(hsc_HeadPitch, lab_HeadPitch, jointpos_if->head_pitch());
    update_servo_value(hsc_RShoulderPitch, lab_RShoulderPitch, jointpos_if->r_shoulder_pitch());
    update_servo_value(hsc_RShoulderRoll, lab_RShoulderRoll, jointpos_if->r_shoulder_roll());
    update_servo_value(hsc_RElbowYaw, lab_RElbowYaw, jointpos_if->r_elbow_yaw());
    update_servo_value(hsc_RElbowRoll, lab_RElbowRoll, jointpos_if->r_elbow_roll());
    update_servo_value(hsc_RWristYaw, lab_RWristYaw, jointpos_if->r_wrist_yaw());
    update_servo_value(hsc_RHand, lab_RHand, jointpos_if->r_hand());
    update_servo_value(hsc_LShoulderPitch, lab_LShoulderPitch, jointpos_if->l_shoulder_pitch());
    update_servo_value(hsc_LShoulderRoll, lab_LShoulderRoll, jointpos_if->l_shoulder_roll());
    update_servo_value(hsc_LElbowYaw, lab_LElbowYaw, jointpos_if->l_elbow_yaw());
    update_servo_value(hsc_LElbowRoll, lab_LElbowRoll, jointpos_if->l_elbow_roll());
    update_servo_value(hsc_LWristYaw, lab_LWristYaw, jointpos_if->l_wrist_yaw());
    update_servo_value(hsc_LHand, lab_LHand, jointpos_if->l_hand());
    update_servo_value(hsc_RHipYawPitch, lab_RHipYawPitch, jointpos_if->r_hip_yaw_pitch());
    update_servo_value(hsc_RHipPitch, lab_RHipPitch, jointpos_if->r_hip_pitch());
    update_servo_value(hsc_RHipRoll, lab_RHipRoll, jointpos_if->r_hip_roll());
    update_servo_value(hsc_RKneePitch, lab_RKneePitch, jointpos_if->r_knee_pitch());
    update_servo_value(hsc_RAnklePitch, lab_RAnklePitch, jointpos_if->r_ankle_pitch());
    update_servo_value(hsc_RAnkleRoll, lab_RAnkleRoll, jointpos_if->r_ankle_roll());
    update_servo_value(hsc_LHipYawPitch, lab_LHipYawPitch, jointpos_if->l_hip_yaw_pitch());
    update_servo_value(hsc_LHipPitch, lab_LHipPitch, jointpos_if->l_hip_pitch());
    update_servo_value(hsc_LHipRoll, lab_LHipRoll, jointpos_if->l_hip_roll());
    update_servo_value(hsc_LKneePitch, lab_LKneePitch, jointpos_if->l_knee_pitch());
    update_servo_value(hsc_LAnklePitch, lab_LAnklePitch, jointpos_if->l_ankle_pitch());
    update_servo_value(hsc_LAnkleRoll, lab_LAnkleRoll, jointpos_if->l_ankle_roll());

    bool currently_servo_enabled = servos_enabled();

    if ( currently_servo_enabled && ! servo_enabled ) {
      tb_stiffness->set_stock_id(Gtk::Stock::YES);
      tb_control->set_sensitive(true);
      tb_getup->set_sensitive(true);
      tb_parkpos->set_sensitive(true);
      tb_zeroall->set_sensitive(true);
    } else if (! currently_servo_enabled && servo_enabled ) {
      tb_stiffness->set_stock_id(Gtk::Stock::NO);
      tb_control->set_sensitive(false);
      tb_getup->set_sensitive(false);
      tb_parkpos->set_sensitive(false);
      tb_zeroall->set_sensitive(false);
    }
    servo_enabled = currently_servo_enabled;
  } catch (Exception &e) {
    // ignored, happens on disconnect while events are pending
  }
}



/** Update sensor values.
 * Called whenever the NaoSensorInterface changes.
 * @param force true to force an update, false to update depending
 * on internal state
 */
void
NaoGuiGtkWindow::update_sensor_values(bool force)
{
  if ( ! sensor_if ) return;

  try {
    sensor_if->read();

    update_sensor_value(lab_l_fsr_fl, sensor_if->l_fsr_fl());
    update_sensor_value(lab_l_fsr_fr, sensor_if->l_fsr_fr());
    update_sensor_value(lab_l_fsr_rl, sensor_if->l_fsr_rl());
    update_sensor_value(lab_l_fsr_rr, sensor_if->l_fsr_rr());
    update_sensor_value(lab_r_fsr_fl, sensor_if->r_fsr_fl());
    update_sensor_value(lab_r_fsr_fr, sensor_if->r_fsr_fr());
    update_sensor_value(lab_r_fsr_rl, sensor_if->r_fsr_rl());
    update_sensor_value(lab_r_fsr_rr, sensor_if->r_fsr_rr());
    update_sensor_value(lab_r_total_weight, sensor_if->r_total_weight());
    update_sensor_value(lab_l_total_weight, sensor_if->l_total_weight());

    update_sensor_value(lab_chest_button, sensor_if->chest_button(), false);
    update_sensor_value(lab_touch_front, sensor_if->head_touch_front(), false);
    update_sensor_value(lab_touch_middle, sensor_if->head_touch_middle(), false);
    update_sensor_value(lab_touch_rear, sensor_if->head_touch_rear(), false);
    update_sensor_value(lab_l_bumper_l, sensor_if->l_foot_bumper_l(), false);
    update_sensor_value(lab_l_bumper_r, sensor_if->l_foot_bumper_r(), false);
    update_sensor_value(lab_r_bumper_l, sensor_if->r_foot_bumper_l(), false);
    update_sensor_value(lab_r_bumper_r, sensor_if->r_foot_bumper_r(), false);

    update_sensor_value(lab_accel_x, sensor_if->accel_x());
    update_sensor_value(lab_accel_y, sensor_if->accel_y());
    update_sensor_value(lab_accel_z, sensor_if->accel_z());
    update_sensor_value(lab_gyro_x, sensor_if->gyro_x());
    update_sensor_value(lab_gyro_y, sensor_if->gyro_y());
    update_sensor_value(lab_gyro_ref, sensor_if->gyro_ref());

    update_sensor_value(lab_battery_charge, sensor_if->battery_charge());

    Glib::ustring l_cop =
      "(" + convert_float2str(sensor_if->l_cop_x(), 1) + ", " + 
      convert_float2str(sensor_if->l_cop_y(), 1) + ")";
    lab_l_cop->set_text(l_cop);

    Glib::ustring r_cop =
      "(" + convert_float2str(sensor_if->r_cop_x(), 1) + ", " + 
      convert_float2str(sensor_if->r_cop_y(), 1) + ")";
    lab_r_cop->set_text(r_cop);

    Glib::ustring angles_xy =
      convert_float2str(sensor_if->angle_x(), 2) + "/" + 
      convert_float2str(sensor_if->angle_y(), 2);
    lab_angles_xy->set_text(angles_xy);


    update_sensor_value(lab_ultrasonic_distance, sensor_if->ultrasonic_distance());
    update_ultrasonic_direction(sensor_if->ultrasonic_direction());

    if ( but_us_auto->get_active() ) {
      NaoSensorInterface::EmitUltrasonicWaveMessage *m =
	new NaoSensorInterface::EmitUltrasonicWaveMessage(cmb_us_direction->get_active_row_number());
      sensor_if->msgq_enqueue(m);
    }
  } catch (Exception &e) {
    // ignored, happens on disconnect while events are pending
  }
}

bool
NaoGuiGtkWindow::servos_enabled() const
{
  jointstiff_if->read();
  return (jointstiff_if->minimum() > 0.);
}


void
NaoGuiGtkWindow::on_stiffness_clicked()
{
  const float stiffness = servos_enabled() ? 0. : 1.;
  NaoJointStiffnessInterface::SetBodyStiffnessMessage *dmsg =
    new NaoJointStiffnessInterface::SetBodyStiffnessMessage(stiffness, 0.5);
  jointstiff_if->msgq_enqueue(dmsg);
}


/** Control toggle event handler. */
void
NaoGuiGtkWindow::on_control_toggled()
{
  bool sensitive = tb_control->get_active();
  hsc_HeadYaw->set_sensitive(sensitive);
  hsc_HeadPitch->set_sensitive(sensitive);
  hsc_RShoulderPitch->set_sensitive(sensitive);
  hsc_RShoulderRoll->set_sensitive(sensitive);
  hsc_RElbowYaw->set_sensitive(sensitive);
  hsc_RElbowRoll->set_sensitive(sensitive);
  hsc_RWristYaw->set_sensitive(sensitive);
  hsc_RHand->set_sensitive(sensitive);
  hsc_LShoulderPitch->set_sensitive(sensitive);
  hsc_LShoulderRoll->set_sensitive(sensitive);
  hsc_LElbowYaw->set_sensitive(sensitive);
  hsc_LElbowRoll->set_sensitive(sensitive);
  hsc_LWristYaw->set_sensitive(sensitive);
  hsc_LHand->set_sensitive(sensitive);
  hsc_RHipYawPitch->set_sensitive(sensitive);
  hsc_RHipPitch->set_sensitive(sensitive);
  hsc_RHipRoll->set_sensitive(sensitive);
  hsc_RKneePitch->set_sensitive(sensitive);
  hsc_RAnklePitch->set_sensitive(sensitive);
  hsc_RAnkleRoll->set_sensitive(sensitive);
  hsc_LHipYawPitch->set_sensitive(sensitive);
  hsc_LHipPitch->set_sensitive(sensitive);
  hsc_LHipRoll->set_sensitive(sensitive);
  hsc_LKneePitch->set_sensitive(sensitive);
  hsc_LAnklePitch->set_sensitive(sensitive);
  hsc_LAnkleRoll->set_sensitive(sensitive);

  if ( ! sensitive ) {
    update_jointpos_values();
  }
}


void
NaoGuiGtkWindow::on_sv_copy_clicked()
{
  std::stringstream txt; 
  txt <<
    "head_yaw = " << (hsc_HeadYaw->get_value() / 100.f) << ",\n" <<
    "head_pitch = " << (hsc_HeadPitch->get_value() / 100.f) << ",\n" <<
    "l_shoulder_pitch = " << (hsc_LShoulderPitch->get_value() / 100.f) << ",\n" <<
    "l_shoulder_roll = " << (hsc_LShoulderRoll->get_value() / 100.f) << ",\n" <<
    "l_elbow_yaw = " << (hsc_LElbowYaw->get_value() / 100.f) << ",\n" <<
    "l_elbow_roll = " << (hsc_LElbowRoll->get_value() / 100.f) << ",\n" <<
    "l_wrist_yaw = " << (hsc_LWristYaw->get_value() / 100.f) << ",\n" <<
    "l_hand = " << (hsc_LHand->get_value() / 100.f) << ",\n" <<
    "r_shoulder_pitch = " << (hsc_RShoulderPitch->get_value() / 100.f) << ",\n" <<
    "r_shoulder_roll = " << (hsc_RShoulderRoll->get_value() / 100.f) << ",\n" <<
    "r_elbow_yaw = " << (hsc_RElbowYaw->get_value() / 100.f) << ",\n" <<
    "r_elbow_roll = " << (hsc_RElbowRoll->get_value() / 100.f) << ",\n" <<
    "r_wrist_yaw = " << (hsc_RWristYaw->get_value() / 100.f) << ",\n" <<
    "r_hand = " << (hsc_RHand->get_value() / 100.f) << ",\n" <<
    "l_hip_yaw_pitch = " << (hsc_LHipYawPitch->get_value() / 100.f) << ",\n" <<
    "l_hip_roll = " << (hsc_LHipRoll->get_value() / 100.f) << ",\n" <<
    "l_hip_pitch = " << (hsc_LHipPitch->get_value() / 100.f) << ",\n" <<
    "l_knee_pitch = " << (hsc_LKneePitch->get_value() / 100.f) << ",\n" <<
    "l_ankle_pitch = " << (hsc_LAnklePitch->get_value() / 100.f) << ",\n" <<
    "l_ankle_roll = " << (hsc_LAnkleRoll->get_value() / 100.f) << ",\n" <<
    "r_hip_yaw_pitch = " << (hsc_RHipYawPitch->get_value() / 100.f) << ",\n" <<
    "r_hip_roll = " << (hsc_RHipRoll->get_value() / 100.f) << ",\n" <<
    "r_hip_pitch = " << (hsc_RHipPitch->get_value() / 100.f) << ",\n" <<
    "r_knee_pitch = " << (hsc_RKneePitch->get_value() / 100.f) << ",\n" <<
    "r_ankle_pitch = " << (hsc_RAnklePitch->get_value() / 100.f) << ",\n" <<
    "r_ankle_roll = " << (hsc_RAnkleRoll->get_value() / 100.f);
  Gtk::Clipboard::get()->set_text(txt.str());
}


void
NaoGuiGtkWindow::on_us_emit_clicked()
{
  NaoSensorInterface::EmitUltrasonicWaveMessage *m =
    new NaoSensorInterface::EmitUltrasonicWaveMessage(cmb_us_direction->get_active_row_number());
  sensor_if->msgq_enqueue(m);
}


void
NaoGuiGtkWindow::on_us_auto_toggled()
{
  but_us_emit->set_sensitive(! but_us_auto->get_active());
}

/** Event handler for connection button. */
void
NaoGuiGtkWindow::on_connection_clicked()
{
  if ( ! connection_dispatcher.get_client()->connected() ) {
    ServiceChooserDialog ssd(*this, connection_dispatcher.get_client());
    ssd.run_and_connect();
  } else {
    connection_dispatcher.get_client()->disconnect();
  }
}

/** Event handler for connected event. */
void
NaoGuiGtkWindow::on_connect()
{
  try {
    bb = new RemoteBlackBoard(connection_dispatcher.get_client());
    jointpos_if =
      bb->open_for_reading<NaoJointPositionInterface>("Nao Joint Positions");
    jointstiff_if =
      bb->open_for_reading<NaoJointStiffnessInterface>("Nao Joint Stiffness");
    sensor_if =
      bb->open_for_reading<NaoSensorInterface>("Nao Sensors");
    nao_navi_if = bb->open_for_reading<NavigatorInterface>("Navigator");
    hummot_fawkes_if = bb->open_for_reading<HumanoidMotionInterface>("Nao Motion");
    hummot_naoqi_if =
      bb->open_for_reading<HumanoidMotionInterface>("NaoQi Motion");

    ifd_jointpos = new InterfaceDispatcher("NaoJointPosIfaceDisp", jointpos_if);
    ifd_sensor = new InterfaceDispatcher("NaoSensorIfaceDisp", sensor_if);
    ifd_jointpos->signal_data_changed().connect(sigc::hide(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::update_jointpos_values), false)));
    ifd_sensor->signal_data_changed().connect(sigc::hide(sigc::bind(sigc::mem_fun(*this, &NaoGuiGtkWindow::update_sensor_values), false)));
    bb->register_listener(ifd_jointpos, BlackBoard::BBIL_FLAG_DATA);
    bb->register_listener(ifd_sensor, BlackBoard::BBIL_FLAG_DATA);

    tb_connection->set_stock_id(Gtk::Stock::DISCONNECT);

    frm_servos->set_sensitive(true);
    frm_sensors->set_sensitive(true);
    frm_ultrasonic->set_sensitive(true);
    tb_stiffness->set_sensitive(true);
    but_us_auto->set_sensitive(true);
    but_us_emit->set_sensitive(true);
    cmb_us_direction->set_sensitive(true);

    this->set_title(std::string("Nao GUI @ ") + connection_dispatcher.get_client()->get_hostname());
  } catch (Exception &e) {
    Glib::ustring message = *(e.begin());
    Gtk::MessageDialog md(*this, message, /* markup */ false,
			  Gtk::MESSAGE_ERROR, Gtk::BUTTONS_OK,
			  /* modal */ true);
    md.set_title("BlackBoard connection failed");
    md.run();
    if ( bb ) {
      bb->unregister_listener(ifd_jointpos);
      bb->unregister_listener(ifd_sensor);
      bb->close(jointpos_if);
      bb->close(jointstiff_if);
      bb->close(sensor_if);
      bb->close(nao_navi_if);
      bb->close(hummot_fawkes_if);
      bb->close(hummot_naoqi_if);
      delete ifd_jointpos;
      delete ifd_sensor;
      delete bb;
      jointpos_if = NULL;
      jointstiff_if = NULL;
      sensor_if = NULL;
      nao_navi_if = NULL;
      hummot_fawkes_if = NULL;
      hummot_naoqi_if = NULL;
      bb = NULL;
      ifd_jointpos = NULL;
      ifd_sensor = NULL;
    }

    connection_dispatcher.get_client()->disconnect();
  }
}

/** Event handler for disconnected event. */
void
NaoGuiGtkWindow::on_disconnect()
{
  if (tb_control->get_active()) tb_control->set_active(false);
  frm_servos->set_sensitive(false);
  frm_sensors->set_sensitive(false);
  frm_ultrasonic->set_sensitive(false);
  tb_stiffness->set_sensitive(false);
  tb_control->set_sensitive(false);
  tb_getup->set_sensitive(false);
  tb_parkpos->set_sensitive(false);
  tb_zeroall->set_sensitive(false);
  but_us_auto->set_sensitive(false);
  but_us_emit->set_sensitive(false);
  cmb_us_direction->set_sensitive(false);

  if (bb) {
    bb->unregister_listener(ifd_jointpos);
    bb->unregister_listener(ifd_sensor);
    bb->close(jointpos_if);
    bb->close(jointstiff_if);
    bb->close(sensor_if);
    bb->close(nao_navi_if);
    bb->close(hummot_fawkes_if);
    bb->close(hummot_naoqi_if);
    delete ifd_jointpos;
    delete ifd_sensor;
    delete bb;
    jointpos_if = NULL;
    jointstiff_if = NULL;
    sensor_if = NULL;
    nao_navi_if = NULL;
    hummot_fawkes_if = NULL;
    hummot_naoqi_if = NULL;
    bb = NULL;
    ifd_jointpos = NULL;
    ifd_sensor = NULL;
  }

  tb_connection->set_stock_id(Gtk::Stock::CONNECT);
  if (servo_enabled) {
    servo_enabled = false;
    tb_stiffness->set_stock_id(Gtk::Stock::NO);
  }
  this->set_title("Nao GUI");
}


void
NaoGuiGtkWindow::on_exit_clicked()
{
  Gtk::Main::quit();
}

void
NaoGuiGtkWindow::on_goto_parkpos_clicked()
{
  if (hummot_naoqi_if && hummot_naoqi_if->has_writer() ) {
    HumanoidMotionInterface::ParkMessage *m =
      new HumanoidMotionInterface::ParkMessage();
    hummot_naoqi_if->msgq_enqueue(m);
  }
}

/**
 * Sets all servos to zeros (calibration configuration)
 */
void
NaoGuiGtkWindow::on_goto_zero_all_clicked()
{
  if ( jointpos_if && jointpos_if->has_writer() ) {
    // NaoJointPositionInterface::GotoAnglesMessage *m =
    //   new NaoJointPositionInterface::GotoAnglesMessage(/*sec */ 3.0,
    //                                               NaoHardwareInterface::INTERPOLATION_SMOOTH,
    //                                               /* head */ 0., 0.,
    //                                               /* l shoulder */ 0., 0.,
    //                                               /* l elbow */ 0., 0.,
    //                                               /* l hip */ 0., 0., 0.,
    //                                               /* l knee */ 0.,
    //                                               /* l ankle */ 0., 0.,
    //                                               /* r hip */ 0., 0., 0.,
    //                                               /* r knee */ 0.,
    //                                               /* r ankle */ 0., 0.,
    //                                               /* r shoulder */ 0., 0.,
    //                                               /* r elbow */ 0., 0.);
    // jointpos_if->msgq_enqueue(m);
  }
}

void
NaoGuiGtkWindow::on_get_up_clicked()
{
  if (hummot_naoqi_if && hummot_naoqi_if->has_writer() ) {
    HumanoidMotionInterface::GetUpMessage *m =
      new HumanoidMotionInterface::GetUpMessage();
    hummot_naoqi_if->msgq_enqueue(m);
  }
}


/**
 * Converts a float value to a Glib::ustring (locale dependent)
 * @param f The float value
 * @param width The precision width
 * @return the formatted string
 */
Glib::ustring
NaoGuiGtkWindow::convert_float2str(float f, unsigned int width)
{
#if GLIBMM_MAJOR_VERSION > 2 || ( GLIBMM_MAJOR_VERSION == 2 && GLIBMM_MINOR_VERSION >= 16 )
  return Glib::ustring::format(std::fixed, std::setprecision(width), f);
#else
  std::ostringstream ss;
  ss << std::fixed << std::setprecision(width);
  ss << f;

  return Glib::locale_to_utf8(ss.str());
#endif
}



bool
NaoGuiGtkWindow::convert_str2float(Glib::ustring sn, float *f)
{
  char *endptr = NULL;
  *f = strtof(sn.c_str(), &endptr);
  if ( endptr[0] != 0 ) {
    Glib::ustring s("Could not convert string to valid number: ");
    s.append(sn, 0, sn.length() - strlen(endptr));
    s += "   &gt;&gt;&gt;<b>";
    s += endptr[0];
    s += "</b>&lt;&lt;&lt;   ";
    s.append(endptr + 1, strlen(endptr) - 1);

    Gtk::MessageDialog md(*this, s,
			  /* use markup */ true,
			  Gtk::MESSAGE_ERROR);
    md.set_title("Invalid value");
    md.run();
    md.hide();
    return false;
  } else {
    return true;
  }
}

void
NaoGuiGtkWindow::on_ws_exec_clicked()
{
  float f;
  if ( convert_str2float(ent_ws_distance->get_text(), &f) ) {
    HumanoidMotionInterface::WalkStraightMessage *m =
      new HumanoidMotionInterface::WalkStraightMessage(f);
    if ( rad_motion_fawkes->get_active() ) {
      hummot_fawkes_if->msgq_enqueue(m);
    } else {
      hummot_naoqi_if->msgq_enqueue(m);
    }
  }
}


void
NaoGuiGtkWindow::on_walkvel_exec_clicked()
{
  float x = 0., y = 0., theta = 0., speed = 0.;
  if ( convert_str2float(ent_walkvel_x->get_text(), &x) &&
       convert_str2float(ent_walkvel_y->get_text(), &y) &&
       convert_str2float(ent_walkvel_theta->get_text(), &theta) &&
       convert_str2float(ent_walkvel_speed->get_text(), &speed) )
  {
    HumanoidMotionInterface::WalkVelocityMessage *m =
      new HumanoidMotionInterface::WalkVelocityMessage(x, y, theta, speed);
    if ( rad_motion_fawkes->get_active() ) {
      hummot_fawkes_if->msgq_enqueue(m);
    } else {
      hummot_naoqi_if->msgq_enqueue(m);
    }
  }
}


void
NaoGuiGtkWindow::on_kick_exec_clicked()
{
  float f;
  if ( convert_str2float(ent_kick_strength->get_text(), &f) ) {
    HumanoidMotionInterface::LegEnum leg = HumanoidMotionInterface::LEG_LEFT;
    if ( cmb_kick_leg->get_active_row_number() == 1 ) {
      leg = HumanoidMotionInterface::LEG_RIGHT;
    }
    HumanoidMotionInterface::KickMessage *m =
      new HumanoidMotionInterface::KickMessage(leg, f);
    if ( rad_motion_fawkes->get_active() ) {
      hummot_fawkes_if->msgq_enqueue(m);
    } else {
      hummot_naoqi_if->msgq_enqueue(m);
    }
  }
}


void
NaoGuiGtkWindow::on_wsw_exec_clicked()
{
  float f;
  if ( convert_str2float(ent_wsw_distance->get_text(), &f) ) {
    HumanoidMotionInterface::WalkSidewaysMessage *m =
      new HumanoidMotionInterface::WalkSidewaysMessage(f);
    if ( rad_motion_fawkes->get_active() ) {
      hummot_fawkes_if->msgq_enqueue(m);
    } else {
      hummot_naoqi_if->msgq_enqueue(m);
    }
  }
}

void
NaoGuiGtkWindow::on_nav_exec_clicked()
{
  float x,y,ori;
  if ( convert_str2float(ent_nav_x->get_text(), &x)
       && convert_str2float(ent_nav_y->get_text(), &y)
       && convert_str2float(ent_nav_ori->get_text(), &ori) ) {
    NavigatorInterface::CartesianGotoMessage *m =
      new NavigatorInterface::CartesianGotoMessage(x, y, ori);
    nao_navi_if->msgq_enqueue(m);
  }
}


void
NaoGuiGtkWindow::on_wa_exec_clicked()
{
  float angle, radius;
  if ( convert_str2float(ent_wa_angle->get_text(), &angle) &&
       convert_str2float(ent_wa_radius->get_text(), &radius) ) {
    HumanoidMotionInterface::WalkArcMessage *m =
      new HumanoidMotionInterface::WalkArcMessage(angle, radius);
    if ( rad_motion_fawkes->get_active() ) {
      hummot_fawkes_if->msgq_enqueue(m);
    } else {
      hummot_naoqi_if->msgq_enqueue(m);
    }
  }
}


void
NaoGuiGtkWindow::on_turn_exec_clicked()
{
  float f;
  if ( convert_str2float(ent_turn_angle->get_text(), &f) ) {
    HumanoidMotionInterface::TurnMessage *m =
      new HumanoidMotionInterface::TurnMessage(f);
    if ( rad_motion_fawkes->get_active() ) {
      hummot_fawkes_if->msgq_enqueue(m);
    } else {
      hummot_naoqi_if->msgq_enqueue(m);
    }
  }
}


void
NaoGuiGtkWindow::on_motion_stop_clicked()
{
  HumanoidMotionInterface::StopMessage *m =
    new HumanoidMotionInterface::StopMessage();
  if ( rad_motion_fawkes->get_active() ) {
    hummot_fawkes_if->msgq_enqueue(m);
  } else {
    hummot_naoqi_if->msgq_enqueue(m);
  }
}


void
NaoGuiGtkWindow::on_stiffness_global_toggled()
{
  bool is_active = chb_stiffness_global->get_active();

  spb_HeadYaw->set_sensitive(! is_active);
  spb_HeadPitch->set_sensitive(! is_active);
  spb_RShoulderPitch->set_sensitive(! is_active);
  spb_RShoulderRoll->set_sensitive(! is_active);
  spb_RElbowYaw->set_sensitive(! is_active);
  spb_RElbowRoll->set_sensitive(! is_active);
  spb_RWristYaw->set_sensitive(! is_active);
  spb_RHand->set_sensitive(! is_active);
  spb_LShoulderPitch->set_sensitive(! is_active);
  spb_LShoulderRoll->set_sensitive(! is_active);
  spb_LElbowYaw->set_sensitive(! is_active);
  spb_LElbowRoll->set_sensitive(! is_active);
  spb_LWristYaw->set_sensitive(! is_active);
  spb_LHand->set_sensitive(! is_active);
  spb_RHipYawPitch->set_sensitive(! is_active);
  spb_RHipPitch->set_sensitive(! is_active);
  spb_RHipRoll->set_sensitive(! is_active);
  spb_RKneePitch->set_sensitive(! is_active);
  spb_RAnklePitch->set_sensitive(! is_active);
  spb_RAnkleRoll->set_sensitive(! is_active);
  spb_LHipYawPitch->set_sensitive(! is_active);
  spb_LHipPitch->set_sensitive(! is_active);
  spb_LHipRoll->set_sensitive(! is_active);
  spb_LKneePitch->set_sensitive(! is_active);
  spb_LAnklePitch->set_sensitive(! is_active);
  spb_LAnkleRoll->set_sensitive(! is_active);

  spb_stiffness_global->set_sensitive(is_active);

  lab_stiff_HeadYaw->set_sensitive(! is_active);
  lab_stiff_HeadPitch->set_sensitive(! is_active);
  lab_stiff_RShoulderPitch->set_sensitive(! is_active);
  lab_stiff_RShoulderRoll->set_sensitive(! is_active);
  lab_stiff_RElbowYaw->set_sensitive(! is_active);
  lab_stiff_RElbowRoll->set_sensitive(! is_active);
  lab_stiff_RWristYaw->set_sensitive(! is_active);
  lab_stiff_RHand->set_sensitive(! is_active);
  lab_stiff_LShoulderPitch->set_sensitive(! is_active);
  lab_stiff_LShoulderRoll->set_sensitive(! is_active);
  lab_stiff_LElbowYaw->set_sensitive(! is_active);
  lab_stiff_LElbowRoll->set_sensitive(! is_active);
  lab_stiff_LWristYaw->set_sensitive(! is_active);
  lab_stiff_LHand->set_sensitive(! is_active);
  lab_stiff_RHipYawPitch->set_sensitive(! is_active);
  lab_stiff_RHipPitch->set_sensitive(! is_active);
  lab_stiff_RHipRoll->set_sensitive(! is_active);
  lab_stiff_RKneePitch->set_sensitive(! is_active);
  lab_stiff_RAnklePitch->set_sensitive(! is_active);
  lab_stiff_RAnkleRoll->set_sensitive(! is_active);
  lab_stiff_LHipYawPitch->set_sensitive(! is_active);
  lab_stiff_LHipPitch->set_sensitive(! is_active);
  lab_stiff_LHipRoll->set_sensitive(! is_active);
  lab_stiff_LKneePitch->set_sensitive(! is_active);
  lab_stiff_LAnklePitch->set_sensitive(! is_active);
  lab_stiff_LAnkleRoll->set_sensitive(! is_active);
}

void
NaoGuiGtkWindow::on_stiffness_write_clicked()
{


  if (chb_stiffness_global->get_active()) {
    float stiff_global = spb_stiffness_global->get_value();
    NaoJointStiffnessInterface::SetBodyStiffnessMessage *m =
      new NaoJointStiffnessInterface::SetBodyStiffnessMessage(stiff_global, .5);
    jointstiff_if->msgq_enqueue(m);
  } else {
    float stiff_head_yaw = spb_HeadYaw->get_value(),
      stiff_head_pitch = spb_HeadPitch->get_value(),
      stiff_l_shoulder_pitch = spb_LShoulderPitch->get_value(),
      stiff_l_shoulder_roll = spb_LShoulderRoll->get_value(),
      stiff_l_elbow_yaw = spb_LElbowYaw->get_value(),
      stiff_l_elbow_roll = spb_LElbowRoll->get_value(),
      stiff_l_wrist_yaw = spb_LWristYaw->get_value(),
      stiff_l_hand = spb_LHand->get_value(),
      stiff_l_hip_yaw_pitch = spb_LHipYawPitch->get_value(),
      stiff_l_hip_pitch = spb_LHipPitch->get_value(),
      stiff_l_hip_roll = spb_LHipRoll->get_value(),
      stiff_l_knee_pitch = spb_LKneePitch->get_value(),
      stiff_l_ankle_roll = spb_LAnkleRoll->get_value(),
      stiff_l_ankle_pitch = spb_LAnklePitch->get_value(),
      stiff_r_shoulder_pitch = spb_RShoulderPitch->get_value(),
      stiff_r_shoulder_roll = spb_RShoulderRoll->get_value(),
      stiff_r_elbow_yaw = spb_RElbowYaw->get_value(),
      stiff_r_elbow_roll = spb_RElbowRoll->get_value(),
      stiff_r_wrist_yaw = spb_RWristYaw->get_value(),
      stiff_r_hand = spb_RHand->get_value(),
      stiff_r_hip_yaw_pitch = spb_RHipYawPitch->get_value(),
      stiff_r_hippitch = spb_RHipPitch->get_value(),
      stiff_r_hiproll = spb_RHipRoll->get_value(),
      stiff_r_knee_pitch = spb_RKneePitch->get_value(),
      stiff_r_ankle_roll = spb_RAnkleRoll->get_value(),
      stiff_r_ankle_pitch = spb_RAnklePitch->get_value();

    NaoJointStiffnessInterface::SetStiffnessesMessage *m =
      new NaoJointStiffnessInterface::SetStiffnessesMessage
             (0.5,
	      stiff_head_yaw, stiff_head_pitch,
	      stiff_l_shoulder_pitch, stiff_l_shoulder_roll,
	      stiff_l_elbow_yaw, stiff_l_elbow_roll,
	      stiff_l_wrist_yaw, stiff_l_hand,
	      stiff_l_hip_yaw_pitch,
	      stiff_l_hip_roll, stiff_l_hip_pitch, stiff_l_knee_pitch,
	      stiff_l_ankle_pitch, stiff_l_ankle_roll,
	      stiff_r_shoulder_pitch, stiff_r_shoulder_roll,
	      stiff_r_elbow_yaw, stiff_r_elbow_roll,
	      stiff_r_wrist_yaw, stiff_r_hand,
	      stiff_r_hip_yaw_pitch, stiff_r_hiproll, stiff_r_hippitch,
	      stiff_r_knee_pitch, stiff_r_ankle_roll, stiff_r_ankle_pitch);

    jointstiff_if->msgq_enqueue(m);
  }
}

void
NaoGuiGtkWindow::on_stiffness_read_clicked()
{
  jointstiff_if->read();

  printf("HeadYaw: %s (%f)\n", convert_float2str(jointstiff_if->head_yaw()).c_str(),
	 jointstiff_if->head_yaw());

  spb_HeadYaw->set_value(jointstiff_if->head_yaw());
  spb_HeadPitch->set_value(jointstiff_if->head_pitch());
  spb_RShoulderPitch->set_value(jointstiff_if->r_shoulder_pitch());
  spb_RShoulderRoll->set_value(jointstiff_if->r_shoulder_roll());
  spb_RElbowYaw->set_value(jointstiff_if->r_elbow_yaw());
  spb_RElbowRoll->set_value(jointstiff_if->r_elbow_roll());
  spb_RWristYaw->set_value(jointstiff_if->r_wrist_yaw());
  spb_RHand->set_value(jointstiff_if->r_hand());
  spb_LShoulderPitch->set_value(jointstiff_if->l_shoulder_pitch());
  spb_LShoulderRoll->set_value(jointstiff_if->l_shoulder_roll());
  spb_LElbowYaw->set_value(jointstiff_if->l_elbow_yaw());
  spb_LElbowRoll->set_value(jointstiff_if->l_elbow_roll());
  spb_LWristYaw->set_value(jointstiff_if->l_wrist_yaw());
  spb_LHand->set_value(jointstiff_if->l_hand());
  spb_RHipYawPitch->set_value(jointstiff_if->r_hip_yaw_pitch());
  spb_RHipPitch->set_value(jointstiff_if->r_hip_pitch());
  spb_RHipRoll->set_value(jointstiff_if->r_hip_roll());
  spb_RKneePitch->set_value(jointstiff_if->r_knee_pitch());
  spb_RAnklePitch->set_value(jointstiff_if->r_ankle_pitch());
  spb_RAnkleRoll->set_value(jointstiff_if->r_ankle_roll());
  spb_LHipYawPitch->set_value(jointstiff_if->l_hip_yaw_pitch());
  spb_LHipPitch->set_value(jointstiff_if->l_hip_pitch());
  spb_LHipRoll->set_value(jointstiff_if->l_hip_roll());
  spb_LKneePitch->set_value(jointstiff_if->l_knee_pitch());
  spb_LAnklePitch->set_value(jointstiff_if->l_ankle_pitch());
  spb_LAnkleRoll->set_value(jointstiff_if->l_ankle_roll());

  spb_stiffness_global->set_value(jointstiff_if->minimum());
}
