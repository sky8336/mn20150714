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
#if !defined(_GYROMETER_CALIBRATED_H_)
#define _GYROMETER_CALIBRATED_H_

#include "Common.h"
#include "SensorManagerTypes.h"
#include "Matrix.h"
#include "platformCalibration.h"

#include "./Sensor.h"

typedef struct _IVH_SENSOR_GYROSCOPE_T
{
    IvhSensor sensor; //keep me first!
    IvhSensorData input;
    IvhSensorData output;

    // This matrix is a set of correction constants that are applied to each
    // gyro sample to rotate it into the correct position for the tilt of the platform
    MATRIX_STRUCT_T CorrectionMatrix;

    // This vector is given by the last row of the correction matrix, and is
    // added to the result of multiplying the correction matrix and the raw
    // gyro result  
    MATRIX_STRUCT_T calibrationOffsets;

    //for AdjustZeroRateOffset
    int32_t s_sampleSumX;
    int32_t s_sampleSumY;
    int32_t s_sampleSumZ;
    uint16_t s_continuousSamplesAtRestCounter;
    float s_prevGyro[3];

    //for filter spike
    uint32_t s_offsetRefinementCount;
    uint16_t s_gyroMotionThreshold;
    uint16_t s_magCovarianceThreshold;

    uint32_t lastX;
    uint32_t lastY;
    uint32_t lastZ;

    PPLATFORM_CALIBRATION_T PlatformCalibration;
    PLATFORM_CALIBRATION_SETTINGS_T* PlatformSettings;
}IvhSensorGyroscope, *pIvhSensorGyroscope;

pIvhSensor CreateSensorGyroscope(const pIvhPlatform platform);
void DestroySensorGyroscope(pIvhSensor me);

#endif /* !defined(_GYROMETER_CALIBRATED_H_) */

