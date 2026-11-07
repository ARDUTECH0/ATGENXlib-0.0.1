#pragma once
// Things that move: a stepper on a STEP/DIR driver, a two-motor robot base, and what a robot steers by.
// Separate from <ATGENXlib.h> because the tilt sensor pulls in Wire.
//
//   #include <ATG_Motion.h>
//   atg::RobotBase car(5, 6, 7, 8, 9, 10);

#include "components/motion/ATG_RobotBase.h"
#include "components/motion/ATG_LineSensor.h"
#include "components/motion/ATG_TiltSensor.h"
#include "components/motion/ATG_Balancer.h"
#include "components/motion/ATG_StepperDriver.h"
