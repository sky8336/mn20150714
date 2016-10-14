////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2011, Intel Corporation.  All Rights Reserved.       //
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
#ifndef _CALIBRATIONCOMMON_H_
#define _CALIBRATIONCOMMON_H_

#include "Common.h"
#include "Error.h"
#include "Matrix.h"
#include "Vector.h"

//Apply a static rotation to the sensor vector about the x-axis (pitch angle) of 80 degrees
//Currently enabled only in WinBlue branch. Disable it for trunk.
//#define ENABLE_APPLY_STATIC_ROTATION

typedef struct
{
    BYTE positionOfX;
    BYTE positionOfY;
    BYTE positionOfZ;
    BOOLEAN reverseX;
    BOOLEAN reverseY;
    BOOLEAN reverseZ;
} CALIBRATION_ORIENTATION_STRUCT_T;

typedef struct
{
    int16_t minX;
    int16_t maxX;
    int16_t minY;
    int16_t maxY;
    int16_t minZ;
    int16_t maxZ;
} CALIBRATION_MINMAX_STRUCT_T;

typedef struct
{
    int16_t offsetX;
    int16_t offsetY;
    int16_t offsetZ;
} CALIBRATION_OFFSET_STRUCT_T;

typedef struct
{
    float scaleX;
    float scaleY;
    float scaleZ;
}  CALIBRATION_SCALE_STRUCT_T;

typedef struct
{
    int32_t rangeX;
    int32_t rangeY;
    int32_t rangeZ;
    int32_t radiusSpheroid;
}  CALIBRATION_RANGE_STRUCT_T;

typedef uint32_t CALIBRATION_ZERO_RATE_THRESHOLD_T;

typedef struct
{
    uint16_t majorAxisLength;
    uint16_t minorAxisLength;
    uint16_t majorAxisAngleDegrees;
}  CALIBRATION_ELLIPSE_PARAMS_STRUCT_T ;

typedef struct
{
    float m11;
    float m21;
    float m31;
    float m12;
    float m22;
    float m32;
    float m13;
    float m23;
    float m33;
    float m10;
    float m20;
    float m30;
} CALIBRATION_CORRECTION_MATRIX_STRUCT_T;


/*******************************************************************************
* Function Name  : CalibrationSwapAxes
* Description    : Swap the orientation of the raw data output from a sensor to 
*                  match the device orientation.  The x,y,z arguments are modified
*                  in this function.
* Return         : void
*
* Convert raw data so that we end up with the following axis orientations
*
*         +z   +y
*          |   /
*          |  /
*          | /
*          .--------- +x
* 
*  Position of X, Y, Z refers to which side of the device the axis is pointing FROM
*  when laying flat on the table
*
*
*  Position 1 = pointing out the top of the device (Z)
*  Position 2 = pointing out the right side of the device (X)
*  Position 3 = pointing out the front side of the device (Y)
*
*******************************************************************************/
void CalibrationSwapAxes(INOUT INT32_VECTOR3_T* const pXyz,
                         IN const CALIBRATION_ORIENTATION_STRUCT_T* const orientStruct);


/*******************************************************************************
* Function Name  : CalibrationApplyOffset
* Description    : Apply offsets to X,Y,Z
* Return         : void
*******************************************************************************/
void CalibrationApplyOffset(INOUT int32_t* const x,
                            INOUT int32_t* const y,
                            INOUT int32_t* const z,
                            IN const CALIBRATION_OFFSET_STRUCT_T* const offsetStruct);


/*******************************************************************************
* Function Name  : CalibrationApplyZeroRateFilter
* Description    : Set values to zero if below the zeroRateThreshold
* Return         : void
*******************************************************************************/
void CalibrationApplyZeroRateFilter(IN const uint32_t zeroRateThreshold,
                                    INOUT int32_t* const x,
                                    INOUT int32_t* const y,
                                    INOUT int32_t* const z);


/*******************************************************************************
* Function Name  : OnePoleLowPassFilter
* Description    : Single pole lowpass filter. Smoothing factor set by input arg alpha
*                : Expression:  y[i] = y[i-1] + alpha * (x[i] - y[i-1])
* Return         : void
*******************************************************************************/

void OnePoleLowPassFilter(IN    float    alpha,
                          IN    int32_t* const x,
                          INOUT int32_t* const yPrevious,
                          INOUT int32_t* const y);


void ApplyLowPassFilter(INOUT float* const data,
                        INOUT float* const prevData,
                        IN uint8_t numElems,
                        IN const float alpha);


ERROR_T CalibrationCopyCorrectionMatrix(IN const CALIBRATION_CORRECTION_MATRIX_STRUCT_T* const correctionMatrix,
                                        INOUT MATRIX_STRUCT_T* const aMatrix,
                                        INOUT MATRIX_STRUCT_T* const  bVector);


void CalibrationApplyCorrectionMatrix(INOUT int32_t* const x,
                                      INOUT int32_t* const y,
                                      INOUT int32_t* const z,
                                      IN const MATRIX_STRUCT_T* const matrixA,
                                      IN const MATRIX_STRUCT_T* const matrixB);


// Test gyro and magnetometer to see if the device is moving
BOOLEAN CalibrationDeviceIsMotionless(IN const float thisGyro[NUM_AXES],
                                      IN const float avgGyro[NUM_AXES],
                                      IN const float magCovariance[COVARIANCE_MATRIX_SIZE],
                                      IN const float gyroMotionThreshold,
                                      IN const float magCovarianceThreshold);
#ifdef ENABLE_APPLY_STATIC_ROTATION
void ApplyStaticRotation(INOUT int32_t* const x,
                         INOUT int32_t* const y,
                         INOUT int32_t* const z);
#endif						 

BOOLEAN CalibrationCorrectionMatrixIsInitialized(IN const MATRIX_STRUCT_T* const correctionMatrix,
                                                 IN const MATRIX_STRUCT_T* const offsets);

void CalibrationSetDefaultCorrectionMatrix(OUT MATRIX_STRUCT_T* const correctionMatrix,
                                           OUT MATRIX_STRUCT_T* const offsets);

#endif /* _CALIBRATIONCOMMON_H_ */

