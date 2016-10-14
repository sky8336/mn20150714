////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2011, Intel Corporation.  All Rights Reserved.	      //
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
//
////////////////////////////////////////////////////////////////////////////////

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _COMPASS_CALIBRATED_H_
#define _COMPASS_CALIBRATED_H_

#include "Error.h"
#include "Common.h"
#include "MagDynamicCali.h"
#include "AccelerometerCalibrated.h"

#include "./Sensor.h"
#include "AccelerometerCalibrated.h"
#include "GyroCalibrated.h"

#define ENABLE_SMOOTH_MAG_SPIKES
#define MAG_FIFO_SIZE 20

struct point {
    int32_t x;
    int32_t y;
    int32_t z;
};

typedef struct _IVH_SENSOR_MAGNATICMETER_T
{
    IvhSensor sensor; //keep me first!
    IvhSensorData input;
    IvhSensorData output;

    IvhSensorData accl;
    IvhSensorData gyro;
    int16_t CALIBRATION_POINT_MAX_INSTANTANEOUS_DIFF;
    //for SmoothMagSpikes
    struct point pointBuffer[3];
    uint8_t currentIndex;
    //forCalculateMagCovariance
    int32_t magFifo[MAG_FIFO_SIZE][NUM_AXES];
    uint8_t magFifoIndex;
    uint32_t rolloverCount;
    pIvhPlatform Platform;
    MAGN_DYNAMICCALI_T Calibration;
}IvhSensorMagnetometer, *pIvhSensorMagnetometer;

pIvhSensor CreateSensorMagnetometer(const pIvhPlatform platform, pIvhSensor accl, pIvhSensor gyro);
void DestroySensorMagnetometer(pIvhSensor me);

#endif /* _COMPASS_CALIBRATED_H_ */


