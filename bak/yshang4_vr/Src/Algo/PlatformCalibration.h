////////////////////////////////////////////////////////////////////////////////
//      Copyright (c) 2012, Intel Corporation.  All Rights Reserved.          //
//                                                                            //
//           Portions derived with permission from STM32Lib                   //
//               Copyright (c) 2010, STMicroelectronics.                      //
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

#if !defined(__PLATFORM_CALIBRATION_H__)
#define __PLATFORM_CALIBRATION_H__
#include "CalibrationCommon.h"

#pragma pack(push)
#pragma pack(4)

typedef struct {
    CALIBRATION_ORIENTATION_STRUCT_T                orientation;
    CALIBRATION_CORRECTION_MATRIX_STRUCT_T          correctionMatrix;
    CALIBRATION_ZERO_RATE_THRESHOLD_T               zeroRateThreshold;
} ACCELEROMETER_CALIBRATION_T;

typedef struct {
    uint32_t    multiplier;    
} ALS_CALIBRATION_T;

typedef struct {
    uint16_t offset1; uint16_t lux1;
    uint16_t offset2; uint16_t lux2;
    uint16_t offset3; uint16_t lux3;
    uint16_t offset4; uint16_t lux4;
    uint16_t offset5; uint16_t lux5;
    uint16_t offset6; uint16_t lux6;
    uint16_t offset7; uint16_t lux7;
    uint16_t offset8; uint16_t lux8;
    uint16_t offset9; uint16_t lux9;
    uint16_t offset10; uint16_t lux10;
} ALR_CURVE_CALIBRATION_T;


typedef struct {
    CALIBRATION_ORIENTATION_STRUCT_T     orientation;
    CALIBRATION_OFFSET_STRUCT_T          offset;
    CALIBRATION_ELLIPSE_PARAMS_STRUCT_T  ellipse;
} MAGNETOMETER_CALIBRATION_T;

typedef struct {
    CALIBRATION_CORRECTION_MATRIX_STRUCT_T   correctionMatrix;
} MAGNETOMETER_3D_CALIBRATION_T;

typedef struct {
    CALIBRATION_ORIENTATION_STRUCT_T    orientation;
    CALIBRATION_OFFSET_STRUCT_T         offset;
    CALIBRATION_ZERO_RATE_THRESHOLD_T   zeroRateThreshold;
} GYROMETER_CALIBRATION_T;

typedef struct {
    CALIBRATION_CORRECTION_MATRIX_STRUCT_T correctionMatrix;
} GYROMETER_CORRECTION_MATRIX_CALIBRATION_T;

typedef struct {
    int16_t correctionN;
    int16_t correctionE;
    int16_t correctionS;
    int16_t correctionW;
} HEADING_CORRECTION_STRUCT_T;

typedef struct {
    HEADING_CORRECTION_STRUCT_T                     correctionZUp;
    HEADING_CORRECTION_STRUCT_T                     correctionZDn;
    HEADING_CORRECTION_STRUCT_T                     correctionXUp;
    HEADING_CORRECTION_STRUCT_T                     correctionXDn;
    HEADING_CORRECTION_STRUCT_T                     correctionYUp;
    HEADING_CORRECTION_STRUCT_T                     correctionYDn;
}  ORIENTATION_HEADING_CORRECTION_STRUCT_T;

typedef struct {
    ACCELEROMETER_CALIBRATION_T                 accel;
    ALS_CALIBRATION_T                           als;
    MAGNETOMETER_CALIBRATION_T                  mag;
    GYROMETER_CALIBRATION_T                     gyro;
    ALR_CURVE_CALIBRATION_T                     alrCurve;
    ACCELEROMETER_CALIBRATION_T                 protractor;
    MAGNETOMETER_3D_CALIBRATION_T               mag3D;
    GYROMETER_CORRECTION_MATRIX_CALIBRATION_T   gyroCorrectionMatrix;
    ORIENTATION_HEADING_CORRECTION_STRUCT_T     headingCorrection;
} PLATFORM_CALIBRATION_T, *PPLATFORM_CALIBRATION_T;

#pragma pack(pop)


//  enableMagnetometerLowPassFilter
//      Reduce magnetometer noise by applying a low pass filter
//
//  enableRestoreCalibrationFromBackupRegisters
//      Store calibration parameters in the backup registers to allow calibration
//      to persist across sensor hub reboots.
//
//  enableAnomalyRejection
//      Detect when the magnetic field exceeds some limits and flag an anomaly
//
typedef struct
{
    BOOLEAN  enableMagnetometerLowPassFilter;
    BOOLEAN  enableMagCalibratedDynamicFilter;
    BOOLEAN  enableAccelerometerLowPassFilter;
} PLATFORM_CALIBRATION_FEATURES_T;


// The behavior of the anomaly detection algorithm is defined 
// by the balance and interrelation among the factors below
//
//  anomalyTriggerPoint
//      The value of the derivative of the norm where we flag anomaly
//
//  magnetometerLowPassFilterAlpha
//      Controls the strength of the low pass filter applied to every mag sample.
//      A value of 0.2 means that each new sample is weighted at 20% of the result.
//
typedef struct
{
    float anomalyTriggerPoint;
    float magLowPassFilterAlpha;
    float gyroBoost;
    int calibration;
    float accLowPassFilterAlpha;
} PLATFORM_CALIBRATION_SETTINGS_T;

typedef struct _PLATFORM_CALIBRATION PLATFORM_CALIBRATION;
typedef struct _MAGN_DYNAMICCALI_T MAGN_DYNAMICCALI_T;

typedef struct _IVH_PLATFORM
{
    PLATFORM_CALIBRATION_FEATURES_T  CalibrationFeatures;
    PLATFORM_CALIBRATION_SETTINGS_T  CalibrationSettings;
    PLATFORM_CALIBRATION_T PlatformCalibration;
    MAGN_DYNAMICCALI_T* Calibrate;
    PLATFORM_CALIBRATION* cali;
}IvhPlatform, *pIvhPlatform;

pIvhPlatform CreatePlatform(PULONG calibrate);
void DestroyPlatform(pIvhPlatform pPlatform);

#endif /* __PLATFORM_CALIBRATION_H__ */
