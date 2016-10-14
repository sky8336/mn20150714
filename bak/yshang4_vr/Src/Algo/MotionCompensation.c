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
// MotionCompensation.cpp:  Logic to estimate motion from accel, magnetometer input
//

#include "Rotation.h"
#include "MotionCompensation.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "Conversion.h"
#include "CalibrationCommon.h"
#include "BackupRegisters.h"
#include "platformCalibration.h"

#include "Trace.h"

static void RotationMatrixMultiplyVector(OUT ROTATION_VECTOR_T* const __restrict mResult,
                                         IN const ROTATION_VECTOR_T* const __restrict mA, 
                                         IN const ROTATION_VECTOR_T* const __restrict mB)
{
    mResult[M11] = (mA[M11] * mB[M11]) + (mA[M12] * mB[M12]) + (mA[M13] * mB[M13]);
    mResult[M12] = (mA[M21] * mB[M11]) + (mA[M22] * mB[M12]) + (mA[M23] * mB[M13]);
    mResult[M13] = (mA[M31] * mB[M11]) + (mA[M32] * mB[M12]) + (mA[M33] * mB[M13]);
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

static void RotationVectorAdd(OUT ROTATION_VECTOR_T* v, IN ROTATION_VECTOR_T* a, IN ROTATION_VECTOR_T* b)
{
    v[0] = a[0] + b[0];
    v[1] = a[1] + b[1];
    v[2] = a[2] + b[2];
}

static void RotationVectorSub(OUT ROTATION_VECTOR_T* v, IN const ROTATION_VECTOR_T* a, IN const ROTATION_VECTOR_T* b)
{
    v[0] = a[0] - b[0];
    v[1] = a[1] - b[1];
    v[2] = a[2] - b[2];
}

static void RotationVectorNorm(OUT ROTATION_DATA_T* const __restrict vResult, IN const ROTATION_VECTOR_T* __restrict const v)
{
    *vResult = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static void RotationMatrixTranspose(OUT ROTATION_MATRIX_T* const __restrict mResult,
                                    IN const ROTATION_MATRIX_T* const __restrict mA)
{
    mResult[M11] = mA[M11];  mResult[M12] = mA[M21];  mResult[M13] = mA[M31];
    mResult[M21] = mA[M12];  mResult[M22] = mA[M22];  mResult[M23] = mA[M32];
    mResult[M31] = mA[M13];  mResult[M32] = mA[M23];  mResult[M33] = mA[M33];
}

typedef enum
{
    SHAKE_INIT,
    SHAKE_STILL,
    SHAKE_SLOW_MOTION,
    SHAKE_FAST_MOTION,
    SHAKE_SHAKING,
    SHAKE_SHAKE_DETECTED //never use!
}IvhShakeStatus;

ERROR_T MotionCompensate(INOUT ROTATION_VECTOR_T* const accel, IN const ROTATION_VECTOR_T* const mag, IN const ROTATION_MATRIX_T* const rotationMatrix)
{
#define ACC_SYSTEM_ERROR 10
#define MAG_SYSTEM_ERROR 30
    ROTATION_MATRIX_T R[9];
    SAFE_MEMCPY_BYTES(R,rotationMatrix,9*sizeof(ROTATION_DATA_T));

    ROTATION_VECTOR_T magu[3]={mag[0], mag[1], mag[2]};
    ROTATION_VECTOR_T accu[3]={accel[0], accel[1], accel[2]};
    RotationVectorScaleToUnitVector(magu);
    RotationVectorMultiply(accu, accu, .001f);
    ROTATION_VECTOR_T da[3] = {accu[0], accu[1], accu[2]};

//...cjy    static ROTATION_VECTOR_T last_acc[3] = {accu[0], accu[1], accu[2]};
//...cjy    static ROTATION_VECTOR_T last_mag[3] = {magu[0], magu[1], magu[2]};
    static ROTATION_VECTOR_T last_acc[3] = {.0f, .0f, -1.f};
    static ROTATION_VECTOR_T last_mag[3] = {.0f, 1.f, .0f};
    //ROTATION_VECTOR_T da[3] = {accu[0] - last_acc[0], accu[1] - last_acc[1], accu[2] - last_acc[2]};
    //ROTATION_VECTOR_T dm[3] = {magu[0] - last_mag[0], magu[1] - last_mag[1], magu[2] - last_mag[2]};
    ROTATION_VECTOR_T rm[3];

    /*shake detect*/
    const ROTATION_VECTOR_T g[3]={.0f,.0f,-1.f};
    ROTATION_VECTOR_T Ag[3];/*acc in global reference frame*/
    ROTATION_VECTOR_T dAg[3];/*delta-acc in global reference frame*/
    ROTATION_DATA_T dAgN;/*norm*/
    RotationMatrixMultiplyVector(Ag, R, accu);
    RotationVectorSub(dAg, Ag, g);
    RotationVectorNorm(&dAgN, dAg);

    //shake detect state machine
    //INIT->SLOW->FAST->SHAKING->FAST->SLOW->DETECT!
    //TODO: need consider ts...500ms...200ms
    static IvhShakeStatus ShakeStatus = SHAKE_INIT;
    if(dAgN > 2.f)//shake detected!
    {
        ShakeStatus = SHAKE_SHAKING;
    }
    else if(dAgN > .5f) //fast motion
    {
        ShakeStatus = SHAKE_FAST_MOTION;
    }
    else if(dAgN > .05f) //slow motion
    {
        ShakeStatus = SHAKE_SLOW_MOTION;
    }
    else //almost keep still
    {
        ShakeStatus = SHAKE_STILL;
    }
    // compensate according to different status
    if(ShakeStatus == SHAKE_STILL)
    {
        return ERROR_OK;
    }
    else if(ShakeStatus == SHAKE_SHAKING)
    {
        //TODO: tell fusion I'm shaking... will calcuate according only mag in a short time...
        //we're blessing the shaking status will only occur in a short time
        return ERROR_OK;
    }

    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "a(%i,%i,%i)Ag(%i,%i,%i),dAgN=%i",int(accu[0]*10000),int(accu[1]*10000),int(accu[2]*10000),int(Ag[0]*10000),int(Ag[1]*10000),int(Ag[2]*10000),int(dAgN*10000));
    //now we're in motion status...
    /*check gimbal lock*/
    RotationVectorCrossProduct(rm, last_mag, magu);
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "m(%i,%i,%i)rm(%i,%i,%i)", int(magu[0]*10000),int(magu[1]*10000),int(magu[2]*10000),int(rm[0]*10000),int(rm[1]*10000),int(rm[2]*10000));
    const ROTATION_DATA_T Kx=5.0f, Ky=-4.0f;
    ROTATION_MATRIX_T Rt[9];
    ROTATION_VECTOR_T Rm1[3], Rm0[3], Rm[3];
    RotationMatrixMultiplyVector(Rm0, R, last_mag);
    RotationMatrixMultiplyVector(Rm1, R, magu);
    RotationVectorCrossProduct(Rm, Rm0, Rm1);
    Rm[0]*=Rm[0],Rm[1]*=Rm[1],Rm[2]*=Rm[2];
    ROTATION_VECTOR_T dA[3]={Rm[2]*Kx, Rm[2]*Ky, .0f};
    RotationMatrixTranspose(Rt, R);
    RotationMatrixMultiplyVector(da, Rt, dA);
    RotationVectorAdd(accu, accu, da);

    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "a(%i,%i,%i)->a(%i,%i,%i)",int(accu[0]*10000),int(accu[1]*10000),int(accu[2]*10000),int((accu[0]-da[0])*10000),int((accu[1]-da[1])*10000),int((accu[2]-da[2])*10000));
    last_acc[0] = accu[0];
    last_acc[1] = accu[1];
    last_acc[2] = accu[2];
    last_mag[0] = magu[0];
    last_mag[1] = magu[1];
    last_mag[2] = magu[2];
    //accel[0] = accu[0]*1000.f;
    //accel[1] = accu[1]*1000.f;
    //accel[2] = accu[2]*1000.f;
    return ERROR_OK;
}
