
/***************************************************************************
 *  robot.h - Fawkes to OpenRAVE Robot Handler
 *
 *  Created: Mon Sep 20 14:50:34 2010
 *  Copyright  2010  Bahram Maleki-Fard, AllemaniACs RoboCup Team
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

#ifndef __OPENRAVE_ROBOT_H
#define __OPENRAVE_ROBOT_H

#include "types.h"

#include <rave/rave.h>
#include <vector>


namespace fawkes {
#if 0 /* just to make Emacs auto-indent happy */
}
#endif

class Logger;
class OpenRAVEManipulator;
class OpenRAVEEnvironment;

class OpenRAVERobot
{
 public:
  OpenRAVERobot(fawkes::Logger* logger = 0);
  OpenRAVERobot(const std::string& filename, fawkes::OpenRAVEEnvironment* env, fawkes::Logger* logger = 0);
  virtual ~OpenRAVERobot();

  // build/load robot parts
  virtual void load(const std::string& filename, fawkes::OpenRAVEEnvironment* env);
  virtual void setReady();
  virtual void calibrate(float& deviceTransX, float& deviceTransY, float& deviceTransZ);
  virtual void setManipulator(fawkes::OpenRAVEManipulator* manip);
  virtual void updateManipulator(); // not needed


  virtual bool setTargetQuat	 (float& transX, float& transY, float& transZ, float& quatW, float& quatX, float& quatY, float& quatZ);
  virtual bool setTargetAxisAngle(float& transX, float& transY, float& transZ, float& angle, float& axisX, float& axisY, float& axisZ);
  virtual bool setTargetEuler(euler_rotation_t type, float& transX, float& transY, float& transZ, float& phi, float& theta, float& psi);
  virtual void setTargetAngles( std::vector<float>& angles ); // just temporary

  virtual void getTargetAngles(std::vector<float>& to); // not needed
  virtual OpenRAVE::RobotBasePtr getRobotPtr() const;
  virtual OpenRAVE::PlannerBase::PlannerParametersPtr getPlannerParams() const;
  virtual std::vector< std::vector<float> >* getTrajectory() const;
  virtual std::vector< std::vector<float> >* getTrajectoryDevice() const;

 private:
  void init();
  bool setTargetTransform(OpenRAVE::Vector& trans, OpenRAVE::Vector& rotQuat);
  bool setTargetEuler(OpenRAVE::Vector& trans, std::vector<float>& rotations);

  fawkes::Logger*	                __logger;

  OpenRAVE::RobotBasePtr                __robot;
  std::string                           __name;
  OpenRAVEManipulator*	                __manip;
  OpenRAVEManipulator*	                __manipGoal;
  OpenRAVE::RobotBase::ManipulatorPtr   __arm;

  OpenRAVE::PlannerBase::PlannerParametersPtr   __plannerParams;
  std::vector< std::vector<float> >*            __traj;
  std::vector<float>                            __anglesTarget;

  float         __transOffsetX;
  float         __transOffsetY;
  float         __transOffsetZ;
};

} // end of namespace fawkes

#endif