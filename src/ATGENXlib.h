#pragma once

#include "core/ATG_Config.h"
#include "core/ATG_Types.h"
#include "core/ATG_Time.h"
#include "core/ATG_Log.h"
#include "core/ATG_EventBus.h"
#include "core/ATG_Scheduler.h"
#include "core/ATG_Module.h"
#include "core/ATG_Runtime.h"
#include "core/ATG_StateMachine.h"

#include "boards/ATG_Board.h"

// Components base
#include "components/base/ATG_ComponentBase.h"
#include "components/base/ATG_DigitalInput.h"
#include "components/base/ATG_DigitalOutput.h"

// Sensors
#include "components/sensors/ATG_DHT.h"
#include "components/sensors/ATG_PushButton.h"
#include "components/sensors/ATG_PIR.h"
#include "components/sensors/ATG_FlameDigital.h"
#include "components/sensors/ATG_ReedSwitch.h"
#include "components/sensors/ATG_IrObstacle.h"
#include "components/sensors/ATG_SoundDigital.h"

// Actuators
#include "components/actuators/ATG_LED.h"
#include "components/actuators/ATG_Relay1Ch.h"
#include "components/actuators/ATG_ActiveBuzzer.h"