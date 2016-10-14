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
// Rotation.cpp:  Logic to calculate rotation matrix from accel, gyro, magnetometer input
//

#include "Rotation.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "Conversion.h"
#include "CalibrationCommon.h"
#include "BackupRegisters.h"
#include "platformCalibration.h"

#include "Trace.h"

void InitRotationParameters(PROTATION_STRUCT rotation)
{
    TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "%!FUNC! Entry");
    // Drift correction factors.  These may be peeked or poked from the
    // shell while running for experimentation purposes
    rotation->WeightAccel = 18;
    rotation->WeightYaw = 16;
    rotation->KProportional = 1.0f;
    rotation->KIntegral = .0f;

    // If the gyro samples are farther apart than this time then we
    // won't evaluate that sample since we're integrating gyro over time
    rotation->MAX_TIME_BETWEEN_GYRO_SAMPLES = 1;

    SAFE_MEMSET(rotation->RotationMatrix,BKP_UNINITIALIZED_VALUE);

    rotation->LastRotationCalcTime = 0;
    TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "%!FUNC! Exit");
}

static void ResetRotationParameters(PROTATION_STRUCT rotation)
{
    SAFE_MEMSET(rotation->RotationMatrix,BKP_UNINITIALIZED_VALUE);

    rotation->LastRotationCalcTime = 0;
}

static BOOLEAN EqualsWithinTolerance(const ROTATION_DATA_T value1, 
                                     const ROTATION_DATA_T value2, 
                                     const ROTATION_DATA_T tolerance)
{
    ROTATION_DATA_T absVal = value1 - value2;
    if (absVal < 0) 
    { 
        absVal *= ROTATION_NEGATIVE_ONE;
    }

    if (absVal < tolerance)
    {
        return TRUE;
    } 
    return FALSE;
}

void ConvertRotationToEulerAngles(IN const ROTATION_MATRIX_T* const rotationMatrix,
                                  OUT ROTATION_DATA_T* const yaw,
                                  OUT ROTATION_DATA_T* const pitch, 
                                  OUT ROTATION_DATA_T* const roll)
{  
    // There are two possible values for pitch, roll, yaw that map to the
    // rotation matrix.
    ROTATION_DATA_T pitchRadians;
    ROTATION_DATA_T rollRadians;
    ROTATION_DATA_T yawRadians;

    // rotationMatrix[M32] == sin(pitch)
    pitchRadians = asinf(rotationMatrix[M32]);
    rollRadians = 0;

    // Check gimbal lock cases first.  rotationMatrix[32] gives the cos of the angle between
    // earth Z and device Y.  If the cos of that angle approaches one or negative one,
    // then we are nearly vertical.  If we're vertical, roll will be zero, and the yaw will be the
    // angle between the earth X and device Z
    if (EqualsWithinTolerance(rotationMatrix[M32], ROTATION_NEGATIVE_ONE, ROTATION_DATA_TOLERANCE))
    {
        yawRadians = -1 * atan2f(rotationMatrix[M13], rotationMatrix[M11]);
    }
    else if (EqualsWithinTolerance(rotationMatrix[M32], ROTATION_ONE, ROTATION_DATA_TOLERANCE))
    {
        yawRadians = atan2f(rotationMatrix[M13], rotationMatrix[M11]);
    } 
    else
    {  
        // If the test for rotationMatrix[M32] == +-1 was correct
        // then cos(pitch) == 0 should never happen
        ROTATION_DATA_T cosPitch = cosf(pitchRadians);   
        ASSERT(!EqualsWithinTolerance(cosPitch, 0, ROTATION_DATA_TOLERANCE));       
        rollRadians = atan2f(ROTATION_NEGATIVE_ONE * rotationMatrix[M31] / cosPitch, 
            rotationMatrix[M33] / cosPitch);

        yawRadians = atan2f( ROTATION_NEGATIVE_ONE * rotationMatrix[M12] / cosPitch,
            rotationMatrix[M22] / cosPitch);
    }

    *yaw = yawRadians * RAD_TO_DEG;
    *pitch = pitchRadians * RAD_TO_DEG;
    *roll = rollRadians * RAD_TO_DEG;

    // convert pitch from +-90 to +- 180
    if (90.0f < fabs(*roll)) // upside down?
    {
        // If upside down, rotate yaw by 180
        *yaw = 180 + *yaw;
        if (0 < *pitch) // tilted up
        {
            *pitch = 180 - *pitch;
        } 
        else  // tilted down
        {
            *pitch = -180 - *pitch;
        }
    }

    // convert roll from +-180 to +- 90
    if (90 < *roll && 180 >= *roll)
    {
        *roll = 180 - *roll;
    }
    else if (-90 > *roll && -180 <= *roll)
    {
        *roll = -180 - *roll;
    }

    // Normalize and convert yaw to 0-360 counterclockwise for MSFT convention
    *yaw = ConversionNormalizeDegrees(360 + *yaw);
}

void ConvertRotationToQuaternion(IN const ROTATION_MATRIX_T* const rotationMatrix,
                                 OUT ROTATION_QUATERNION_T* const quaternion)
{
    //
    //Copyright (c) 2006 - 2011 The Open Toolkit library.
    //Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), 
    //to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, 
    //and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions
    //
    //The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
    //
    //THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
    //MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
    //CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE 
    //OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
    //
    //Modified by Intel 2011
    //

    double trace, S, X, Y, Z, W;

    // Note: replace sqrt() with Taylor series
    // Note: replace floating pt math with fixed pt

    trace = (double)(ROTATION_ONE + rotationMatrix[M11] + rotationMatrix[M22] + rotationMatrix[M33]);

    if (trace > 0) 
    {
        S =  sqrt(trace) * 2.0;
        X = (rotationMatrix[M32] - rotationMatrix[M23]) / S;
        Y = (rotationMatrix[M13] - rotationMatrix[M31]) / S;
        Z = (rotationMatrix[M21] - rotationMatrix[M12]) / S;
        W = 0.25 * S;
    } 
    else 
    { 
        if(rotationMatrix[M11] > rotationMatrix[M22] && rotationMatrix[M11] > rotationMatrix[M33])    
        {
            S = sqrt(ROTATION_ONE + rotationMatrix[M11] - rotationMatrix[M22] - rotationMatrix[M33]) * 2.0; 
            X = 0.25 * S;
            Y = (rotationMatrix[M12] + rotationMatrix[M21]) / S;
            Z = (rotationMatrix[M13] + rotationMatrix[M31]) / S;        
            W = (rotationMatrix[M32] - rotationMatrix[M23]) / S;
        } 
        else if (rotationMatrix[M22] > rotationMatrix[M33])
        {
            S = sqrt(ROTATION_ONE + rotationMatrix[M22] - rotationMatrix[M11] - rotationMatrix[M33]) * 2.0;
            X = (rotationMatrix[M12] + rotationMatrix[M21]) / S;
            Y = 0.25 * S;
            Z = (rotationMatrix[M23] + rotationMatrix[M32]) / S;
            W = (rotationMatrix[M13] - rotationMatrix[M31]) / S;
        } 
        else 
        {       
            S = sqrt(ROTATION_ONE + rotationMatrix[M33] - rotationMatrix[M11] - rotationMatrix[M22]) * 2.0;
            X = (rotationMatrix[M13] + rotationMatrix[M31]) / S;
            Y = (rotationMatrix[M23] + rotationMatrix[M32]) / S;
            Z = 0.25 * S;
            W = (rotationMatrix[M21] - rotationMatrix[M12]) / S;
        }
    }

    // Normalize if necessary
    double magnitudeSqr = X*X + Y*Y + Z*Z + W*W;
    const double QUATERNION_NORMALIZATION_TOLERANCE = 1e-6;
    if (fabs(1 - magnitudeSqr) > QUATERNION_NORMALIZATION_TOLERANCE)
    {
        double magnitude = sqrt(magnitudeSqr);
        X /= magnitude;
        Y /= magnitude;
        Z /= magnitude;
        W /= magnitude;
    }

    quaternion[QX] = (float)X;
    quaternion[QY] = (float)Y;
    quaternion[QZ] = (float)Z;
    quaternion[QW] = (float)W;
}

ERROR_T CalculateRotationUsingAccel(IN const ROTATION_VECTOR_T* const accel,
                                    IN const ROTATION_VECTOR_T* const mag,
                                    OUT ROTATION_MATRIX_T* const rotationMatrix)
{
    ERROR_T retVal = ERROR_OK;
    float pitchRadians, rollRadians, yawRadians = 0.0;

    // Find pitch (angle of rotation about the X axis)
    // and roll (angle of rotation about the Y axis)
    // Multiply accelZ by ROTATION_NEGATIVE_ONE to get positive 1G when held flat
    // The convention when doing tilt compensation is to limit pitch to +- 90 degrees
    // so we use atanf for pitch.
    // atan2f produces results +- 180 degrees, so we use it for yaw and roll
    // Note that this is different than the Microsoft convention for inclinometer
    // so further mapping must be done in the inclinometer microdriver
    if (0 == accel[AXIS_X] && 0 == accel[AXIS_Z])
    {
        rollRadians = 0;
    }
    else
    {
        rollRadians = atan2f(accel[AXIS_X], (ROTATION_NEGATIVE_ONE * accel[AXIS_Z]));
    }
    float sinRoll = sinf(rollRadians);
    float cosRoll = cosf(rollRadians);

    float pitchDenominator = (accel[0] * sinRoll) + (ROTATION_NEGATIVE_ONE * accel[2] * cosRoll);
    if (0 == accel[AXIS_Y] && 0 == pitchDenominator)
    {
        pitchRadians = 0;
    }
    else
    {
        pitchRadians = ROTATION_NEGATIVE_ONE * atanf(accel[AXIS_Y] / pitchDenominator);    
    }
    float sinPitch = sinf(pitchRadians);
    float cosPitch = cosf(pitchRadians);

    // Get the yaw angle.
    // We're getting compensated magX and magY from calibrated compass
    if (0 == mag[AXIS_X] && 0 == mag[AXIS_Y])
    {
        yawRadians = 0;
    }
    else
    {
        yawRadians = atan2f( mag[AXIS_X], mag[AXIS_Y]);
    }

    float sinYaw = sinf(yawRadians);
    float cosYaw = cosf(yawRadians);

    rotationMatrix[M11] = (cosYaw * cosRoll) - (sinYaw * sinPitch * sinRoll);    
    rotationMatrix[M12] = ROTATION_NEGATIVE_ONE * sinYaw * cosPitch;
    rotationMatrix[M13]= (cosYaw * sinRoll) + (sinYaw * sinPitch * cosRoll);
    rotationMatrix[M21] = (sinYaw * cosRoll) + (cosYaw * sinPitch* sinRoll);
    rotationMatrix[M22] = cosYaw * cosPitch;
    rotationMatrix[M23] = (sinYaw * sinRoll ) - (cosYaw * sinPitch *cosRoll);
    rotationMatrix[M31] = ROTATION_NEGATIVE_ONE * cosPitch * sinRoll;
    rotationMatrix[M32] = sinPitch;
    rotationMatrix[M33] = cosPitch * cosRoll;

    return retVal;
}

ERROR_T CalculateDCM(IN const ROTATION_VECTOR_T* const accel,
                     IN const ROTATION_VECTOR_T* const mag,
                     OUT ROTATION_MATRIX_T* const rotationMatrix)
{
    ERROR_T retVal = ERROR_OK;
    float a[]={accel[0],accel[1],accel[2]};
    float m[]={mag[0],mag[1],mag[2]};
    float an=sqrtf(a[0]*a[0]+a[1]*a[1]+a[2]*a[2]);
    float mn=sqrtf(m[0]*m[0]+m[1]*m[1]+m[2]*m[2]);
    a[0]/=an,a[1]/=an,a[2]/=an;
    m[0]/=mn,m[1]/=mn,m[2]/=mn;
    float t[]={a[1]*m[2]-a[2]*m[1],a[2]*m[0]-a[0]*m[2],a[0]*m[1]-a[1]*m[0]};
    m[0]=t[1]*a[2]-t[2]*a[1],m[1]=t[2]*a[0]-t[0]*a[2],m[2]=t[0]*a[1]-t[1]*a[0];
    mn=sqrtf(m[0]*m[0]+m[1]*m[1]+m[2]*m[2]);
    m[0]/=mn,m[1]/=mn,m[2]/=mn;
    t[0]=a[1]*m[2]-a[2]*m[1],t[1]=a[2]*m[0]-a[0]*m[2],t[2]=a[0]*m[1]-a[1]*m[0];

    rotationMatrix[0]=t[0];
    rotationMatrix[1]=t[1];
    rotationMatrix[2]=t[2];
    rotationMatrix[3]=m[0];
    rotationMatrix[4]=m[1];
    rotationMatrix[5]=m[2];
    rotationMatrix[6]=-a[0];
    rotationMatrix[7]=-a[1];
    rotationMatrix[8]=-a[2];

    return retVal;
}

void RotationMatrixMultiply(OUT ROTATION_VECTOR_T* const __restrict mResult,
                            IN const ROTATION_VECTOR_T* const __restrict mA, 
                            IN const ROTATION_VECTOR_T* const __restrict mB)
{
    mResult[M11] = (mA[M11] * mB[M11]) + (mA[M12] * mB[M21]) + (mA[M13] * mB[M31]);
    mResult[M12] = (mA[M11] * mB[M12]) + (mA[M12] * mB[M22]) + (mA[M13] * mB[M32]);
    mResult[M13] = (mA[M11] * mB[M13]) + (mA[M12] * mB[M23]) + (mA[M13] * mB[M33]);

    mResult[M21] = (mA[M21] * mB[M11]) + (mA[M22] * mB[M21]) + (mA[M23] * mB[M31]);
    mResult[M22] = (mA[M21] * mB[M12]) + (mA[M22] * mB[M22]) + (mA[M23] * mB[M32]);
    mResult[M23] = (mA[M21] * mB[M13]) + (mA[M22] * mB[M23]) + (mA[M23] * mB[M33]);

    mResult[M31] = (mA[M31] * mB[M11]) + (mA[M32] * mB[M21]) + (mA[M33] * mB[M31]);
    mResult[M32] = (mA[M31] * mB[M12]) + (mA[M32] * mB[M22]) + (mA[M33] * mB[M32]);
    mResult[M33] = (mA[M31] * mB[M13]) + (mA[M32] * mB[M23]) + (mA[M33] * mB[M33]);
}

static void RotationMatrixMultiplyVector(OUT ROTATION_VECTOR_T* const __restrict mResult,
                                         IN const ROTATION_VECTOR_T* const __restrict mA, 
                                         IN const ROTATION_VECTOR_T* const __restrict mB)
{
    mResult[M11] = (mA[M11] * mB[M11]) + (mA[M12] * mB[M12]) + (mA[M13] * mB[M13]);
    mResult[M12] = (mA[M21] * mB[M11]) + (mA[M22] * mB[M12]) + (mA[M23] * mB[M13]);
    mResult[M13] = (mA[M31] * mB[M11]) + (mA[M32] * mB[M12]) + (mA[M33] * mB[M13]);
}



static void RotationMatrixTranspose(OUT ROTATION_MATRIX_T* const __restrict mResult,
                                    IN const ROTATION_MATRIX_T* const __restrict mA)
{
    mResult[M11] = mA[M11];  mResult[M12] = mA[M21];  mResult[M13] = mA[M31];
    mResult[M21] = mA[M12];  mResult[M22] = mA[M22];  mResult[M23] = mA[M32];
    mResult[M31] = mA[M13];  mResult[M32] = mA[M23];  mResult[M33] = mA[M33];
}

static void RotationVectorDotProduct(OUT ROTATION_DATA_T* const __restrict result,
                                     IN const ROTATION_VECTOR_T* __restrict const vA,
                                     IN const ROTATION_VECTOR_T* __restrict const vB)
{
    *result = (vA[0] * vB[0]) + (vA[1] * vB[1]) + (vA[2] * vB[2]);
}

static void RotationVectorCrossProduct(OUT ROTATION_VECTOR_T* const __restrict vResult,
                                       IN const ROTATION_VECTOR_T* __restrict const vA,
                                       IN const ROTATION_VECTOR_T* __restrict const vB)
{
    vResult[0] = (vA[1] * vB[2]) - (vA[2] * vB[1]);
    vResult[1] = (vA[2] * vB[0]) - (vA[0] * vB[2]);
    vResult[2] = (vA[0] * vB[1]) - (vA[1] * vB[0]);
}

static void RotationVectorMultiply(OUT ROTATION_VECTOR_T* const __restrict vResult,
                                   IN const ROTATION_VECTOR_T* const __restrict v,
                                   IN ROTATION_DATA_T scale)
{
    vResult[0] = v[0] * scale;
    vResult[1] = v[1] * scale;
    vResult[2] = v[2] * scale;
}

static void RotationVectorScaleToUnitVector(INOUT ROTATION_VECTOR_T* v)
{
    ROTATION_DATA_T scaleFactor = 1.0f;
    if(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] > .00001f)
    {
        scaleFactor = ROTATION_ONE / (sqrt(v[0] * v[0] +
            v[1] * v[1] +
            v[2] * v[2]));
    }

    RotationVectorMultiply(v, v, scaleFactor);
}

// Use the current rotation matrix to find the part of the accel vector
// that is due to gravity.  The gravity vector is useful as a correction
// input into the rotation matrix.
static void FindGravityVector(IN const ROTATION_VECTOR_T* const accel,
                              IN const ROTATION_MATRIX_T* const rotationMatrix,
                              OUT ROTATION_VECTOR_T* const gVector)
{
    UNREFERENCED_PARAMETER(rotationMatrix);
    // Apply a low pass filter to isolate gravity
    static ROTATION_VECTOR_T gPrev[3] = {0, 0, -1000};
    const float k = 0.3f;
    memcpy(gVector, accel, 3 * sizeof(ROTATION_VECTOR_T));
    ApplyLowPassFilter(gVector, gPrev, 3, k);
}


static void CalculateRollPitchCorrection(IN PROTATION_STRUCT rotation,
                                         IN const ROTATION_VECTOR_T* const accel,
                                         IN const ROTATION_MATRIX_T* const rotationMatrix,
                                         OUT ROTATION_VECTOR_T* const rollPitchCorrection)
{
    ROTATION_DATA_T wAccel = 0;

    ROTATION_VECTOR_T gVector[3] = {0, 0, 0};
    FindGravityVector(accel, rotationMatrix, gVector);

    float accelDiffFromG = sqrtf((accel[0] - gVector[0]) * (accel[0] - gVector[0]) +
        (accel[1] - gVector[1]) * (accel[1] - gVector[1]) +
        (accel[2] - gVector[2]) * (accel[2] - gVector[2]));

    // If the the accel vector varies from the gravity vector
    // it means the device is being moved.
    // We only want to correct gyro drift when the device is at rest.

    // For small accelDiffFromG values less than ACCEL_DIFF_FROM_G_THRESHOLD, use the full weight
    // of the accel correction.  As accelDiffFromG increases, rapidly ramp down wAccel
    const float ACCEL_DIFF_FROM_G_THRESHOLD = 2.5;
    if (accelDiffFromG < ACCEL_DIFF_FROM_G_THRESHOLD)
    {
        wAccel = rotation->WeightAccel;
    }
    else
    {
        wAccel = rotation->WeightAccel / (accelDiffFromG / ACCEL_DIFF_FROM_G_THRESHOLD);
    }

    // Flip the sign of Z so that G is positive when flat
    ROTATION_VECTOR_T accelUnitVector[3];
    RotationVectorMultiply(accelUnitVector, accel, ROTATION_NEGATIVE_ONE);
    RotationVectorScaleToUnitVector(accelUnitVector);

    // See Premerlani Eqn. 27
    // The cross product of accel and the Z row of the rotation matrix
    // creates a correction vector that we can apply to omega
    //
    // The cross product gives you a vector perpendicular to the two input vectors
    // with magnitude proportional to the magnitudes of the input vectors times the
    // sin of the angle between them.
    // 
    // Because the rotation matrix and accel are both unit vectors of magnitude 1
    // the cross product gives us the sin between the two vectors
    RotationVectorCrossProduct(rollPitchCorrection,
        accelUnitVector, 
        (ROTATION_VECTOR_T *)&rotationMatrix[M31]);

    // Apply the accel weight
    RotationVectorMultiply(rollPitchCorrection, rollPitchCorrection, wAccel);
}


static void CalculateYawCorrection(IN PROTATION_STRUCT rotation,
                                   IN const ROTATION_VECTOR_T* const mag,
                                   IN const float magConfidence,
                                   IN const ROTATION_MATRIX_T* rotationMatrix,
                                   OUT ROTATION_VECTOR_T* yawCorrection)
{
    ROTATION_DATA_T wYaw = 0;
    UNREFERENCED_PARAMETER(rotationMatrix);
    UNREFERENCED_PARAMETER(magConfidence);

    // Only apply yaw correction when there is no anomaly detected
    ///we must apply yaw correction even when mag is anomaly to avoid drift.
    //if (abs(1.0f - magConfidence) < 0.0001f)
    {
        wYaw = rotation->WeightYaw;

        // Get the unit vector for the calibrated magnetometer
        ROTATION_VECTOR_T magUnitVector[3] = {mag[0], mag[1], mag[2]};
        RotationVectorScaleToUnitVector(magUnitVector);

        // Use the rotation matrix to convert the measured magnetic vector in the body frame
        // to Earth frame
        ROTATION_VECTOR_T magVectorEarthFrame[3] = {0, 0, 0};
        RotationMatrixMultiplyVector(magVectorEarthFrame, rotation->RotationMatrix, magUnitVector);
        // Throw away Z to disregard magnetic inclination
        magVectorEarthFrame[2] = 0;
        // Scale the modified vector to a unit vector
        RotationVectorScaleToUnitVector(magVectorEarthFrame);

        // Magnetic North in the earth frame is aligned with the positive Y axis
        const ROTATION_VECTOR_T refNorthEarthFrame[3] = {0, 1, 0};       
        ROTATION_VECTOR_T yawCorrectionEarthFrame[] = {0, 0, 0};
        // The cross product of the modified mag vector and the reference North vector in earth frame
        // creates a correction vector that we can apply to omega
        // Cross product will create a vector in Z with the sin of the angle
        // between the mag vector and the reference North vector
        RotationVectorCrossProduct(yawCorrectionEarthFrame,
            magVectorEarthFrame,
            refNorthEarthFrame);

        // Get the transpose of the rotation matrix to convert the
        // correction vector back to body frame
        ROTATION_MATRIX_T rmatTranspose[ROTATION_MATRIX_NUM_ELEMS];
        RotationMatrixTranspose(rmatTranspose, rotation->RotationMatrix);

        // Rotate the modified vector back to body frame
        RotationMatrixMultiplyVector(yawCorrection, rmatTranspose, yawCorrectionEarthFrame);
    }

    RotationVectorMultiply(yawCorrection, yawCorrection, wYaw);
}

static void RotationDriftCorrection(IN PROTATION_STRUCT rotation,
                                    IN const ROTATION_VECTOR_T* const accel,
                                    IN const ROTATION_VECTOR_T* const mag,
                                    IN const ROTATION_VECTOR_T* const gyro,
                                    IN const float magConfidence,
                                    IN const ROTATION_MATRIX_T* const rotationMatrix,
                                    IN const ROTATION_DATA_T dtGyro,
                                    IN const ROTATION_DATA_T dtAccel,
                                    OUT ROTATION_VECTOR_T* const driftCorrection)
{
    ROTATION_VECTOR_T rollPitchCorrection[] = {0, 0, 0};
    ROTATION_VECTOR_T yawCorrection[] = {0, 0, 0};
    UNREFERENCED_PARAMETER(dtAccel);
    UNREFERENCED_PARAMETER(gyro);

    CalculateRollPitchCorrection(rotation, accel, rotationMatrix, rollPitchCorrection);
    CalculateYawCorrection(rotation, mag, magConfidence, rotationMatrix, yawCorrection);

    // Integral correction accumulates during runtime so it's static    
    static ROTATION_VECTOR_T integralCorrection[] = {0, 0, 0}; 
    ROTATION_VECTOR_T proportionalCorrection[] = {0, 0, 0};            

    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]rp(%f,%f,%f)y(%f,%f,%f)",rollPitchCorrection[0],rollPitchCorrection[1],rollPitchCorrection[2],yawCorrection[0],yawCorrection[1],yawCorrection[2]);

    int i;
    for (i=0; i<3; ++i)
    {
        proportionalCorrection[i] = rollPitchCorrection[i] + yawCorrection[i];
        integralCorrection[i] += rotation->KIntegral * dtGyro * proportionalCorrection[i];

        // Apply roll, pitch, and yaw proportional and integral corrections
        driftCorrection[i] = (rotation->KProportional * proportionalCorrection[i]) + integralCorrection[i];
    }
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]p=%f,i=%f,dt=%f,drift(%f,%f,%f)",rotation->KProportional,rotation->KIntegral,dtGyro,driftCorrection[0],driftCorrection[1],driftCorrection[2]);
}

static ERROR_T RotationNormalize(INOUT ROTATION_MATRIX_T* const rotationMatrix)
{
    ERROR_T retVal = ERROR_OK;

    ROTATION_DATA_T error;

    // The dot product of any two axes is supposed to be 0 if they are orthogonal to each other.
    // Find the normalization error by computing the dot-product of the X and Y rows of the matrix
    ROTATION_VECTOR_T* rowX = &rotationMatrix[M11];
    ROTATION_VECTOR_T* rowY = &rotationMatrix[M21];
    ROTATION_VECTOR_T* rowZ = &rotationMatrix[M31];
    RotationVectorDotProduct(&error, rowX, rowY);

    // Apply half the error correction to both X and Y
    ROTATION_VECTOR_T tmpX[] = {0, 0, 0};
    ROTATION_DATA_T halfError = 0.5f * error;
    tmpX[0] = rowX[0] - (halfError * rowY[0]);
    tmpX[1] = rowX[1] - (halfError * rowY[1]);
    tmpX[2] = rowX[2] - (halfError * rowY[2]);

    rowY[0] = rowY[0] - (halfError * rowX[0]);
    rowY[1] = rowY[1] - (halfError * rowX[1]);
    rowY[2] = rowY[2] - (halfError * rowX[2]);

    memcpy(rowX, tmpX, sizeof(tmpX));

    // Now that rowX and rowY are orthogonal, we can get an orthogonal Z
    // axis by setting the Z axis to the cross product of rowX and rowY
    RotationVectorCrossProduct(rowZ, rowX, rowY);

    // Normalize each row so that the magnitude of each vector is ~= 1
    RotationVectorScaleToUnitVector(rowX);
    RotationVectorScaleToUnitVector(rowY);
    RotationVectorScaleToUnitVector(rowZ);   

    return retVal;
}

// Store the rotation matrix in the backup registers so that the
// orientation can be restored after a sensor hub restart.
// The backup registers hold uint16_t, but the rotation matrix is
// a float.  Rotation matrix values are always between 0 and +-1.
// Multiplying by RMAT_BKP_CONVERSION_FACTOR of 10,000 and casting
// is a safe way to convert to uint16_t while still preserving four
// significant digits of precision.
static void StoreRotationMatrixInBkp(IN PROTATION_STRUCT rotation, IN ROTATION_MATRIX_T* const __restrict rotationMatrix)
{
    int i;
    for (i=0; i < ROTATION_MATRIX_NUM_ELEMS; ++i)
    {
        int16_t tmp = (int16_t)(rotationMatrix[M11+i] * rotation->RMAT_BKP_CONVERSION_FACTOR);
        BackupRegisterWrite((BACKUP_REGISTER_T)(BKP_RMAT_M11 + i), *((uint16_t *)&tmp));
    }
}

// Restore the rotation matrix from the backup registers by
// converting the uint16_t to int16_t and then dividing by 
// RMAT_BKP_CONVERSION_FACTOR of 10,000 while casting to float.
static void RestoreRotationMatrixFromBkp(IN PROTATION_STRUCT rotation, OUT ROTATION_MATRIX_T* const __restrict rotationMatrix)
{
    int i;
    for (i=0; i < ROTATION_MATRIX_NUM_ELEMS; ++i)
    {
        uint16_t tmp = BackupRegisterRead((BACKUP_REGISTER_T)(BKP_RMAT_M11 + i));
        int16_t tmp2 = *((int16_t *)&tmp);
        rotationMatrix[M11+i] = (float)((float)tmp2 / rotation->RMAT_BKP_CONVERSION_FACTOR);
    }
}

static BOOLEAN RotationMatrixIsValid(IN const ROTATION_MATRIX_T* rotationMatrix)
{
    BOOLEAN rotationMatrixIsValid = TRUE;
    // Check each row of the rotation matrix for garbage
    // The vector magnitude of each row should be near 1.0
    uint8_t row, col;
    for (row=0; row < ROTATION_MATRIX_NUM_ROWS; ++row)
    {
        float sumSqrs = 0;
        for (col=0; col < ROTATION_MATRIX_NUM_COLS; ++col)
        {
            uint8_t index = (row * ROTATION_MATRIX_NUM_COLS) + col;
            if (FALSE == isfinite(rotationMatrix[index]))
            {
                rotationMatrixIsValid = FALSE;
                break;
            }
            sumSqrs += rotationMatrix[index] * rotationMatrix[index];
        }
        float norm = sqrtf(sumSqrs);
        const float MAX_RMAT_DIFF_FROM_ONE = 0.5;
        if (fabs(norm - 1) > MAX_RMAT_DIFF_FROM_ONE)
        {
            rotationMatrixIsValid = FALSE;
            break;
        }
    }

    return rotationMatrixIsValid;
}

static uint32_t CalculateTimeDifference(const uint32_t currentTime,
                                 const uint32_t prevTime)
{
    uint32_t timeDiff;
    if (currentTime > prevTime)
    {
        timeDiff = currentTime - prevTime;
    }
    else  // handle rollover
    {
        timeDiff = 0xffffffffu - prevTime + currentTime;
    }

    return timeDiff;
}

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
                                   OUT ROTATION_VECTOR_T incrementalRotationDegrees[NUM_AXES]
)
{
    ERROR_T retVal = ERROR_OK;   

    UNREFERENCED_PARAMETER(lastTimestampMag);
    incrementalRotationDegrees[AXIS_X] = 0;
    incrementalRotationDegrees[AXIS_Y] = 0;
    incrementalRotationDegrees[AXIS_Z] = 0;

    // dTheta is the change in angle since the last sample
    // dTheta == rate of rotation in radians per second * dt
    // dt is the interval between samples.  
    ROTATION_VECTOR_T dTheta[3] = {0, 0, 0};

    // Convert ms to s and find the dt for integrating gyro and accel
    float dtGyro = CalculateTimeDifference(lastTimestampGyro, rotation->LastRotationCalcTime) * MILLISECS_TO_SECS;
    float dtAccel = CalculateTimeDifference(lastTimestampAccel, rotation->LastRotationCalcTime) * MILLISECS_TO_SECS;

    // First time only, initialize the rotation matrix using the accel and mag
    if (0 == rotation->LastRotationCalcTime)
    {
        RestoreRotationMatrixFromBkp(rotation, rotation->RotationMatrix);
    }

    // Catch any bad rotation matrix, whether due to bad initialization
    // or some other exception that may have caused the rotation matrix
    // to become invalid
    if (FALSE == RotationMatrixIsValid(rotation->RotationMatrix))
    {
        CalculateRotationUsingAccel(accel, mag, rotation->RotationMatrix);
        StoreRotationMatrixInBkp(rotation, rotation->RotationMatrix);
    }

    if (0 != rotation->LastRotationCalcTime && 
        dtGyro < rotation->MAX_TIME_BETWEEN_GYRO_SAMPLES) 
    {
        ROTATION_VECTOR_T omega[3];
        RotationVectorMultiply(omega, gyro, (MILLIDPS_TO_DPS * DEG_TO_RAD));

        // Correct gyro drift before calculating rotation
        RotationDriftCorrection(rotation, accel, mag, gyro,
            magConfidence,
            rotation->RotationMatrix, dtGyro, dtAccel, driftCorrection);

        omega[AXIS_X] += driftCorrection[AXIS_X];
        omega[AXIS_Y] += driftCorrection[AXIS_Y];
        omega[AXIS_Z] += driftCorrection[AXIS_Z];

        RotationVectorMultiply(dTheta, omega, dtGyro);

        // See Premerlani paper Eqn. 17
        ROTATION_VECTOR_T thisRotation[] = {               1,   -1 * dTheta[AXIS_Z],        dTheta[AXIS_Y],
            dTheta[AXIS_Z],                1,   -1 * dTheta[AXIS_X],
            -1 * dTheta[AXIS_Y],        dTheta[AXIS_X],                1};
        ROTATION_VECTOR_T lastRotation[] = {rotation->RotationMatrix[0], rotation->RotationMatrix[1], rotation->RotationMatrix[2],
            rotation->RotationMatrix[3], rotation->RotationMatrix[4], rotation->RotationMatrix[5],
            rotation->RotationMatrix[6], rotation->RotationMatrix[7], rotation->RotationMatrix[8]};

        // Update the rotation matrix by multiplying the previous matrix 
        // with the current rotation matrix
        RotationMatrixMultiply(rotation->RotationMatrix, lastRotation, thisRotation);
        retVal = RotationNormalize(rotation->RotationMatrix);
        if (ERROR_OK != retVal)
        {
            goto end;
        }

        // Copy the calculated rotation matrix to the output data struct
        memcpy(rotationMatrix, rotation->RotationMatrix, sizeof(rotation->RotationMatrix));

        // Store the current rotation matrix in the backup registers
        StoreRotationMatrixInBkp(rotation, rotation->RotationMatrix);

        // Return the incremental rotation in degrees
        incrementalRotationDegrees[AXIS_X] = dTheta[AXIS_X] * RAD_TO_DEG;
        incrementalRotationDegrees[AXIS_Y] = dTheta[AXIS_Y] * RAD_TO_DEG;
        incrementalRotationDegrees[AXIS_Z] = dTheta[AXIS_Z] * RAD_TO_DEG;
    }

end:                                
    rotation->LastRotationCalcTime = lastTimestampGyro;
    return retVal;
}
