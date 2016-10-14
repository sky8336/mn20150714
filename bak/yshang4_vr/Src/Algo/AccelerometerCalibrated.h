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
//
// Accelerometer_calibrated.h : Logic for the Calibrated Digital Accelerometer.
//

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _ACCELEROMETER_CALIBRATED_H_
#define _ACCELEROMETER_CALIBRATED_H_

#include "SensorManagerTypes.h"
#include "Common.h"
#include "Error.h"
#include "platformCalibration.h"
#include "./Sensor.h"
#include "AccDynamicCali.h"

//------------------------------------------------------------------------------
// DATA
//------------------------------------------------------------------------------
typedef struct _IVH_SENSOR_ACCELEROMETER_T
{
    IvhSensor sensor; //keep me first!
    IvhSensorData input;
    IvhSensorData output;

    // This matrix is a set of correction constants that are applied to each
    // accel sample to rotate it into the correct position for the tilt of the platform
    MATRIX_STRUCT_T CorrectionMatrix;

    // This vector is given by the last row of the correction matrix, and is
    // added to the result of multiplying the correction matrix and the raw
    // accel result  
    MATRIX_STRUCT_T CorrectionVector;
    PPLATFORM_CALIBRATION_T PlatformCalibration;
    pIvhPlatform Platform;
    ACCL_DYNAMICCALI_T Calibration;
}IvhSensorAccelerometer, *pIvhSensorAccelerometer;

pIvhSensor CreateSensorAccelerometer(const pIvhPlatform platform);
void DestroySensorAccelerometer(pIvhSensor me);

#endif /* _ACCELEROMETER_CALIBRATED_H_ */


