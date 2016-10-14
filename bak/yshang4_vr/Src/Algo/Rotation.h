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
// Rotation.h:  Logic to calculate rotation matrix from accel, gyro, magnetometer input
// 

#if !defined(_ROTATION_H_)
#define _ROTATION_H_

#include "Error.h"
#include "math.h"
#include "Common.h"

//------------------------------------------
// STRUCT / DATA DECLARATIONS
//------------------------------------------

// ROTATION_MATRIX_T is a 3x3 matrix of direction cosines
#define ROTATION_MATRIX_NUM_ELEMS 9
#define ROTATION_MATRIX_NUM_ROWS 3
#define ROTATION_MATRIX_NUM_COLS 3

// For now, use float for our rmat data type
// We'll need to convert to fixed point math later for efficiency
typedef float ROTATION_DATA_T;
typedef ROTATION_DATA_T ROTATION_MATRIX_T;
typedef ROTATION_DATA_T ROTATION_QUATERNION_T;
typedef ROTATION_DATA_T ROTATION_VECTOR_T;

typedef struct _ROTATION_STRUCT
{
    // The backup registers hold uint16_t.  The rmat values
    // are floats between zero and 1.  Multiplying by 10000
    // gets us four digits of decimal precision.
    uint16_t RMAT_BKP_CONVERSION_FACTOR;

    // Drift correction factors.  These may be peeked or poked from the
    // shell while running for experimentation purposes
    ROTATION_DATA_T WeightAccel;
    ROTATION_DATA_T WeightYaw;
    ROTATION_DATA_T KProportional;
    ROTATION_DATA_T KIntegral;

    // If the gyro samples are farther apart than this time then we
    // won't evaluate that sample since we're integrating gyro over time
    ROTATION_DATA_T MAX_TIME_BETWEEN_GYRO_SAMPLES;

    ROTATION_MATRIX_T RotationMatrix[ROTATION_MATRIX_NUM_ELEMS];

    int32_t LastRotationCalcTime;
}ROTATION_STRUCT, *PROTATION_STRUCT;

static const ROTATION_DATA_T ROTATION_ONE = 1.0f;
static const ROTATION_DATA_T ROTATION_NEGATIVE_ONE = -1.0f;

static const ROTATION_DATA_T MILLISECS_TO_SECS = 0.001f;
static const ROTATION_DATA_T MILLIDPS_TO_DPS = 0.001f;

static const ROTATION_DATA_T RAD_TO_DEG = 180 / M_PI;
static const ROTATION_DATA_T DEG_TO_RAD = M_PI / 180;

static const ROTATION_DATA_T PITCH_CORRECTION_THRESHOLD = 10.0f;
static const ROTATION_DATA_T ROLL_CORRECTION_THRESHOLD = 10.0f;

// Tolerance around which we consider a floating point number to be zero
// The tolerance is 1 - cos(1 degree) =~ 0.0001
static const ROTATION_DATA_T  ROTATION_DATA_TOLERANCE = 0.0001f;

// static consts for readability when accessing array elements
static const uint8_t M11 = 0;
static const uint8_t M12 = 1;
static const uint8_t M13 = 2;
static const uint8_t M21 = 3;
static const uint8_t M22 = 4;
static const uint8_t M23 = 5;
static const uint8_t M31 = 6;
static const uint8_t M32 = 7;
static const uint8_t M33 = 8;

static const uint8_t QX = 0;
static const uint8_t QY = 1;
static const uint8_t QZ = 2;
static const uint8_t QW = 3;

void RotationMatrixMultiply(OUT ROTATION_VECTOR_T* const __restrict mResult,
                            IN const ROTATION_VECTOR_T* const __restrict mA, 
                            IN const ROTATION_VECTOR_T* const __restrict mB);

void ConvertRotationToEulerAngles(IN const ROTATION_MATRIX_T* const rotationMatrix,
                                  OUT ROTATION_DATA_T* const yaw,
                                  OUT ROTATION_DATA_T* const pitch, 
                                  OUT ROTATION_DATA_T* const roll);

void ConvertRotationToQuaternion(IN const ROTATION_MATRIX_T* const rotationMatrix,
                                 OUT ROTATION_QUATERNION_T* const quaternion);

ERROR_T CalculateRotationUsingGyro(IN PROTATION_STRUCT rotation, 
                                   IN const ROTATION_VECTOR_T* const accel,
                                   IN const ROTATION_VECTOR_T* const gyro,
                                   IN const ROTATION_VECTOR_T* const mag,
                                   IN const int32_t lastTimestampGyro,
                                   IN const int32_t lastTimestampMag,
                                   IN const int32_t lastTimestampAccel,
                                   IN const float magConfidence,
                                   OUT ROTATION_MATRIX_T rotationMatrix[ROTATION_MATRIX_NUM_ELEMS],
                                   OUT ROTATION_VECTOR_T driftCorrection[NUM_AXES],
                                   OUT ROTATION_VECTOR_T incrementalRotationDegrees[NUM_AXES]);

ERROR_T CalculateRotationUsingAccel(IN const ROTATION_VECTOR_T* const accel,
                                    IN const ROTATION_VECTOR_T* const mag,
                                    OUT ROTATION_MATRIX_T* const rotationMatrix);

ERROR_T CalculateDCM(IN const ROTATION_VECTOR_T* const accel,
                     IN const ROTATION_VECTOR_T* const mag,
                     OUT ROTATION_MATRIX_T* const rotationMatrix);

void InitRotationParameters(PROTATION_STRUCT rotation);

#endif // !defined(_ROTATION_H_)
