////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2011, Intel Corporation.  All Rights Reserved.          //
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

/* Define to prevent recursive inclusion -------------------------------------*/
#if !defined(_GYROMETER_CALIBRATED_6AXIS_H_)
#define _GYROMETER_CALIBRATED_6AXIS_H_

#include "Common.h"
#include "SensorManagerTypes.h"
#include "Matrix.h"
#include "platformCalibration.h"

#include "./Sensor.h"

typedef struct _IVH_SENSOR_GYROSCOPE_6AXIS_T
{
    IvhSensor sensor; //keep me first!
    IvhSensorData input;
    IvhSensorData output;

    FLOAT_VECTOR9_T rm0;
    uint32_t t0;
    float last_gyro[5][3];
    int index;
    int32_t last_data[3];
}IvhSensorGyroscope6Axis, *pIvhSensorGyroscope6Axis;

pIvhSensor CreateSensorGyroscope6Axis(const pIvhSensor orientation);
void DestroySensorGyroscope6Axis(pIvhSensor me);

#endif /* !defined(_GYROMETER_CALIBRATED_6AXIS_H_) */

