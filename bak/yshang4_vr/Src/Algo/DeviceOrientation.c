////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2013, Intel Corporation.  All Rights Reserved.       //
//                                                                            //
//              INTEL CORPORATION PROPRIETARY INFORMATION                     //
//                                                                            //
// The source code contained or described herein and all documents related to //
// the source code (Material) are owned by Intel Corporation or its suppliers //
// or licensors. Title to the Material remains with Intel Corporation or its  //
// suppliers and licensors. The Material contains trade secrets and           //
// proprietary and confidential information of Intel or its suppliers and     //
// licensors. The Material is protected by worldwide copyright and trade      //
// secret laws and treaty provisions. No part of the Material may be used,    //
// copied, reproduced, modified, published, uploaded, posted, transmitted,    //
// distributed, or disclosed in any way without Intel’s prior express written //
// permission.                                                                //
//                                                                            //
// No license under any patent, copyright, trade secret or other intellectual //
// property right is granted to or conferred upon you by disclosure or        //
// delivery of the Materials, either expressly, by implication, inducement,   //
// estoppel or otherwise. Any license under such intellectual property rights //
// must be express and approved by Intel in writing.                          //
//                                                                            //
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY      //
// KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE        //
// IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR      //
// PURPOSE.                                                                   //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

#include "DeviceOrientation.h"
#include "TMR.h"
#include "Rotation.h"
#include "stdlib.h"
#include "platformCalibration.h"
#include "CalibrationCommon.h"
#include "Matrix.h"
#include "Rotation.h"
#include "MagDynamicCali.h"
#include "BackupRegisters.h"

#include "AccelerometerCalibrated.h"
#include "GyroCalibrated.h"
#include "CompassCalibrated.h"
#include "CompassOrientation.h"

#include "DeviceOrientation6Axis.h"
#include "DeviceOrientation9Axis.h"

#include "Trace.h"

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------
pIvhSensor CreateSensorOrientation(const pIvhPlatform platform, pIvhSensor accl, pIvhSensor gyro, pIvhSensor magn)
{
    if(gyro == NULL)
    {
        return CreateSensorOrientation6Axis(platform, accl, magn);
    }
    else
    {
        return CreateSensorOrientation9Axis(platform, accl, gyro, magn);
    }
}

void DestroySensorOrientation(pIvhSensor me)
{
    if(me->type == IVH_SENSOR_TYPE_ORIENTATION_6AXIS)
    {
        DestroySensorOrientation6Axis(me);
    }
    else if(me->type == IVH_SENSOR_TYPE_ORIENTATION_9AXIS)
    {
        DestroySensorOrientation9Axis(me);
    }
}
