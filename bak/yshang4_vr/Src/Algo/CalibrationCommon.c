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
//
////////////////////////////////////////////////////////////////////////////////
//
// CalibrationCommon.c : Utility library for calibration driver
//
/* Includes ------------------------------------------------------------------*/
#include "CalibrationCommon.h"
#include "stdlib.h"
#include "math.h"
#include "Error.h"
#include "Vector.h"

#include "Trace.h"
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
                         IN const CALIBRATION_ORIENTATION_STRUCT_T* const orientStruct)
{

    int32_t chipX = pXyz->x;
    int32_t chipY = pXyz->y;
    int32_t chipZ = pXyz->z;

    switch (orientStruct->positionOfX)
    { 
    case 1: // position 1 is supposed to tell us Z
        pXyz->z = chipX;
        break;
    case 2: // position 2 is supposed to tell us X
        pXyz->x = chipX;        
        break;
    case 3: // position 3 is supposed to tell us Y
        pXyz->y = chipX;
        break;
    }

    switch (orientStruct->positionOfY)
    { 
    case 1: // position 1 is supposed to tell us Z
        pXyz->z = chipY;
        break;
    case 2: // position 2 is supposed to tell us X
        pXyz->x = chipY;       
        break;
    case 3: // position 3 is supposed to tell us Y
        pXyz->y = chipY;
        break;
    }

    switch (orientStruct->positionOfZ)
    { 
    case 1: // position 1 is supposed to tell us Z
        pXyz->z = chipZ;
        break;
    case 2: // position 2 is supposed to tell us X
        pXyz->x = chipZ;
        break;
    case 3: // position 3 is supposed to tell us Y
        pXyz->y = chipZ;
        break;
    }

    if (orientStruct->reverseX)
    {
        pXyz->x *= -1;
    }
    if (orientStruct->reverseY)
    {
        pXyz->y *= -1;
    }
    if (orientStruct->reverseZ)
    {
        pXyz->z *= -1;
    }
}

/*******************************************************************************
* Function Name  : CalibrationApplyOffset
* Description    : Apply offsets to X,Y,Z
* Return         : void
*******************************************************************************/
void CalibrationApplyOffset(INOUT int32_t* const x,
                            INOUT int32_t* const y,
                            INOUT int32_t* const z,
                            IN const CALIBRATION_OFFSET_STRUCT_T* const offsetStruct)
{
    *x -= offsetStruct->offsetX;
    *y -= offsetStruct->offsetY;
    *z -= offsetStruct->offsetZ;
}


/*******************************************************************************
* Function Name  : CalibrationApplyZeroRateFilter
* Description    : Set values to zero if below the zeroRateThreshold
* Return         : void
*******************************************************************************/
void CalibrationApplyZeroRateFilter(IN const uint32_t zeroRateThreshold,
                                    INOUT int32_t* const x,
                                    INOUT int32_t* const y,
                                    INOUT int32_t* const z)
{
    if ((uint32_t)(abs(*x)) < zeroRateThreshold)
    {
        *x = 0;
    }
    if ((uint32_t)(abs(*y)) < zeroRateThreshold)
    {
        *y = 0;
    }
    if ((uint32_t)(abs(*z)) < zeroRateThreshold)
    {
        *z = 0;
    }
}

/*******************t************************************************************
* Function Name  : OnePoleLowPassFilter
* Description    : Single pole lowpass filter. Smoothing factor set by input arg alpha
*                : Expression:  y[i] = y[i-1] + alpha * (x[i] - y[i-1])
* Return         : void
*******************************************************************************/

void OnePoleLowPassFilter(IN    float    alpha,
                          IN    int32_t* const x,
                          INOUT int32_t* const yPrevious,
                          INOUT int32_t* const y)
{
    float yOut;

    yOut = *yPrevious + alpha * (*x - *yPrevious);

    *yPrevious = (int32_t) yOut;

    *y   = (int32_t) yOut;
}

void ApplyLowPassFilter(INOUT float* const __restrict data,
                        INOUT float* const __restrict prevData,
                        IN uint8_t numElems,
                        IN const float alpha)
{
    uint8_t i;
    const float alphaRemainder = 1 - alpha;
    for (i=0; i<numElems; ++i)
    {
        data[i] = prevData[i] = 
            (data[i] * alpha) + (alphaRemainder * prevData[i]);
    }
}


ERROR_T CalibrationCopyCorrectionMatrix(IN const CALIBRATION_CORRECTION_MATRIX_STRUCT_T* const correctionMatrix,
                                        INOUT MATRIX_STRUCT_T* const aMatrix,
                                        INOUT MATRIX_STRUCT_T* const  bVector)
{
    ERROR_T retVal = ERROR_OK;

    if (aMatrix->rows != 3 || aMatrix->cols != 3 ||
        bVector->rows != 3 || bVector->cols != 1)
    {
        retVal =  ERROR_BAD_VALUE;
    }
    else
    {
        aMatrix->m[0] = correctionMatrix->m11;
        aMatrix->m[1] = correctionMatrix->m12;
        aMatrix->m[2] = correctionMatrix->m13;
        aMatrix->m[3] = correctionMatrix->m21;
        aMatrix->m[4] = correctionMatrix->m22;
        aMatrix->m[5] = correctionMatrix->m23;
        aMatrix->m[6] = correctionMatrix->m31;
        aMatrix->m[7] = correctionMatrix->m32;
        aMatrix->m[8] = correctionMatrix->m33;

        bVector->m[0] = correctionMatrix->m10;
        bVector->m[1] = correctionMatrix->m20;
        bVector->m[2] = correctionMatrix->m30;
    }

    return retVal;
}

void CalibrationApplyCorrectionMatrix(INOUT int32_t* const x,
                                      INOUT int32_t* const y,
                                      INOUT int32_t* const z,
                                      IN const MATRIX_STRUCT_T* const matrixA,
                                      IN const MATRIX_STRUCT_T* const matrixB)
{
    MATRIX_STRUCT_T sensorVector = {3, 1, {.0f, .0f, .0f}};
    sensorVector.m[0] = (float)*x;
    sensorVector.m[1] = (float)*y;
    sensorVector.m[2] = (float)*z;

    MATRIX_STRUCT_T resultMultiply = {3, 1, {.0f, .0f, .0f}};
    MatrixMultiply(matrixA, &sensorVector, &resultMultiply);

    MATRIX_STRUCT_T resultAdd = {3, 1, {.0f, .0f, .0f}};
    MatrixAdd(&resultMultiply, matrixB, &resultAdd);

    *x = (int32_t)(resultAdd.m[0]+.5f);
    *y = (int32_t)(resultAdd.m[1]+.5f);
    *z = (int32_t)(resultAdd.m[2]+.5f);
}


BOOLEAN CalibrationDeviceIsMotionless(IN const float thisGyro[NUM_AXES],
                                      IN const float avgGyro[NUM_AXES],
                                      IN const float magCovariance[COVARIANCE_MATRIX_SIZE],
                                      IN const float gyroMotionThreshold,
                                      IN const float magCovarianceThreshold)
{


    float thisGyroNorm = sqrtf(thisGyro[AXIS_X] * thisGyro[AXIS_X] +
        thisGyro[AXIS_Y] * thisGyro[AXIS_Y] +
        thisGyro[AXIS_Z] * thisGyro[AXIS_Z]);


    float avgGyroNorm = sqrtf(avgGyro[AXIS_X] * avgGyro[AXIS_X] +
        avgGyro[AXIS_Y] * avgGyro[AXIS_Y] +
        avgGyro[AXIS_Z] * avgGyro[AXIS_Z]);


    // The covariance test checks to see if the components of the magnetometer are moving in
    // relation to each other, or if the just have random noise or jitter.  
    // The covariance matrix is an array containing the upper triangular elements of a 3x3 matrix
    // ex.
    //         [  X_TO_X   X_TO_Y   X_TO_Z   ]       == Array elements [ 0   1   2 ]
    //         [           Y_TO_Y   Y_TO_Z   ]                         [     3   4 ]
    //         [                    Z_TO_Z   ]                         [         5 ]
    //
    // If the mag is moving normally, then the covariance will increase in elements 
    // 1, 2, and 4, which give covariance of X to Y, X to Z, and Y to Z.
    const uint8_t COVARIANCE_X_TO_Y = 1;
    const uint8_t COVARIANCE_X_TO_Z = 2;
    const uint8_t COVARIANCE_Y_TO_Z = 4;
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]cova(%f,%f,%f)",magCovariance[COVARIANCE_X_TO_Y],magCovariance[COVARIANCE_X_TO_Z],magCovariance[COVARIANCE_Y_TO_Z]);

    if ((fabs(avgGyroNorm - thisGyroNorm) < gyroMotionThreshold) &&
        (magCovariance[COVARIANCE_X_TO_Y] < magCovarianceThreshold) &&
        (magCovariance[COVARIANCE_X_TO_Z] < magCovarianceThreshold) &&
        (magCovariance[COVARIANCE_Y_TO_Z] < magCovarianceThreshold))
    {
        return TRUE;
    }

    return FALSE;
}

#ifdef ENABLE_APPLY_STATIC_ROTATION
void ApplyStaticRotation(INOUT int32_t* const x,
                         INOUT int32_t* const y,
                         INOUT int32_t* const z)
{
    float StaticR[] = {1, 0, 0, 0,.173648,.98481,0,-.98481,.173648}; 
    float tempVec[] = {0, 0, 0};

    tempVec[0] = (float)*x;
    tempVec[1] = (float)*y;
    tempVec[2] = (float)*z;

    //Apply a static rotation to the accel vector about the x-axis (pitch angle) of 80 degrees
    //This represents (in theory) the position that the accelerometer would be in if
    //  it were mounted in the lid of a clamshell. 
    //We assume an average angle between the (back of) lid and horizontal of 80 degrees.
    //Since this is currently a fixed rotation, we will use the matrix values computed
    //  for the x-axis rotation as follows:
    //  
    //StaticR  =  1     0       0
    //            0  cos(80) sin(80)
    //            0 -sin(80) cos(80)
    //
    //StaticR  =  1     0       0
    //            0  .173648 .98481
    //            0 -.98481  .173648
    //

    //RotationMatrixMultiplyVector(tempVecRotated,StaticR,tempVec);
    *x = (StaticR[0] * tempVec[0]) + (StaticR[1] * tempVec[1]) + (StaticR[2] * tempVec[2]);
    *y = (StaticR[3] * tempVec[0]) + (StaticR[4] * tempVec[1]) + (StaticR[5] * tempVec[2]);
    *z = (StaticR[6] * tempVec[0]) + (StaticR[7] * tempVec[1]) + (StaticR[8] * tempVec[2]);
}
#endif


BOOLEAN CalibrationCorrectionMatrixIsInitialized(IN const MATRIX_STRUCT_T* const correctionMatrix,
                                                 IN const MATRIX_STRUCT_T* const offsets)
{
    const uint16_t UNINITIALIZED_DFU_VALUE = 0xFFFF;
    int8_t i;
    const uint8_t CORRECTION_MATRIX_NUM_ELEMS = 9;
    BOOLEAN correctionMatrixInitialized = TRUE;
    for (i=0; i < CORRECTION_MATRIX_NUM_ELEMS; ++i)
    {
        if (correctionMatrix->m[i] == UNINITIALIZED_DFU_VALUE)
        {
            correctionMatrixInitialized = FALSE;
        }
    }

    const uint8_t CORRECTION_OFFSETS_NUM_ELEMS = 3;
    BOOLEAN offsetsInitialized = TRUE;
    for (i=0; i < CORRECTION_OFFSETS_NUM_ELEMS; ++i)
    {
        if (offsets->m[i] == UNINITIALIZED_DFU_VALUE)
        {
            offsetsInitialized = FALSE;
        }
    }


    return (correctionMatrixInitialized && offsetsInitialized);
}

void CalibrationSetDefaultCorrectionMatrix(OUT MATRIX_STRUCT_T* const correctionMatrix,
                                           OUT MATRIX_STRUCT_T* const offsets)
{
    // Initialize correctionMatrix to an identity matrix
    correctionMatrix->m[0] = 1;
    correctionMatrix->m[1] = 0;
    correctionMatrix->m[2] = 0;

    correctionMatrix->m[3] = 0;
    correctionMatrix->m[4] = 1;
    correctionMatrix->m[5] = 0;

    correctionMatrix->m[6] = 0;
    correctionMatrix->m[7] = 0;
    correctionMatrix->m[8] = 1;

    // Initialize offsets to zero
    offsets->m[0] = 0;
    offsets->m[1] = 0;
    offsets->m[2] = 0;
}
