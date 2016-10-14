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
// $Archive: /Super Tablet/Intel.SensorHub.Firmware.STM32/DeviceOrientation.h $
//
////////////////////////////////////////////////////////////////////////////////

//
// DeviceOrientation.h : Logic for the DeviceOrientation "fusion" virtual sensor.  It is structured as a "sensor" driver.
//
#if !defined(_DEVICEORIENTATION_H_)
#define _DEVICEORIENTATION_H_

#include "AccelerometerCalibrated.h"
#include "CompassCalibrated.h"
#include "GyroCalibrated.h"
#include "AlsCalibrated.h"
#include "CompassOrientation.h"
#include "Inclinometer.h"
#include "MagDynamicCali.h"

#include "./Sensor.h"

typedef struct _IVH_SENSOR_ORIENTATION_T
{
    IvhSensor sensor; //keep me first!
    IvhSensorData input;
    IvhSensorData output;

    IvhSensorData accl;
    IvhSensorData magn;

    ROTATION_STRUCT Rotation;
    PMAGN_DYNAMICCALI_T Calibration;

    ROTATION_VECTOR_T lastCalibratedAccel[3];
    ROTATION_VECTOR_T lastCalibratedGyro[3];
    ROTATION_VECTOR_T lastCalibratedMag[3];
    ROTATION_VECTOR_T lastRotatedMag[3];
    ROTATION_VECTOR_T lastRawMag[3];
    uint32_t lastCalibratedAls;

    float lastMagCovariance[COVARIANCE_MATRIX_SIZE];

    float magConfidence;
    uint32_t lastTimestampAccel;
    uint32_t lastTimestampGyro;
    uint32_t lastTimestampMag;

    uint8_t estimatedHeadingError;

    pIvhPlatform platform;
    // Don't apply ZRT until at least one sample has been calculated and
    // SensorManager has been notified.
    BOOLEAN atLeastOneSampleCalculated;

    ROTATION_MATRIX_T rotationMatrixStruct[9];

    ROTATION_VECTOR_T driftCorrection[3];

    ROTATION_VECTOR_T s_prevGyro[3]; //for ApplyZRToffset
    uint32_t s_motionlessTime;
    uint32_t s_startMotionlessTime;

    //for dynamic lpf
    BOOLEAN s_firstTime;
    float s_prevMag[NUM_AXES];

    //for calcuate rotation
    float prevGyro[3];
    float prevAccel[3];
    uint32_t previousTimestampMag;
}IvhSensorOrientation, *pIvhSensorOrientation;

//------------------------------------------
// FUNCTION PROTOTYPES
//------------------------------------------

pIvhSensor CreateSensorOrientation(const pIvhPlatform platform, pIvhSensor accl, pIvhSensor gyro, pIvhSensor magn);
void DestroySensorOrientation(pIvhSensor me);

#endif
