////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2013, Intel Corporation.  All Rights Reserved.	      //
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
// distributed, or disclosed in any way without Intel's prior express written //
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

/* Includes ------------------------------------------------------------------*/
#include "Error.h"
#include "math.h"
#include "stdio.h"
#include "TMR.h"
#include "stdint.h"
#include "platformCalibration.h"
#include "CalibrationCommon.h"
#include "stdlib.h"
#include "Rotation.h"
#include "CompassCalibrated.h"
#include "MagDynamicCali.h"
#include "BackupRegisters.h"
#include <float.h>
#include "Matrix.h"
#include "Rotation.h"
#include "Conversion.h"

#include "Trace.h"

//------------------------------------------------------------------------------
// DATA
//------------------------------------------------------------------------------
#define IVH_DYNAMIC_CALIBRATION_POOL_TAG 'DHVI'

static float CalculateVectorNorm(const ROTATION_VECTOR_T* const v)
{
    return sqrtf(v[AXIS_X] * v[AXIS_X] +
        v[AXIS_Y] * v[AXIS_Y] +
        v[AXIS_Z] * v[AXIS_Z]);
}

static void SetAnomalyState(PMAGN_DYNAMICCALI_T cali, BOOLEAN anomalyState)
{
    // Return immediately if no-op
    if (cali->inAnomaly == anomalyState)
    {
        return;
    }

    cali->inAnomaly = anomalyState;

    // Reset the hold counter which makes sure we don't exit anomaly too quickly
    // Reset the anomaly fifos to throw away the initial anomaly data.
    // We'll still adapt to the anomaly if it persists
    if (TRUE == cali->inAnomaly)
    {
        cali->anomalyHoldCount = 0;
    }
}

// fixed it! see document
static int CalcuateOffset(ROTATION_VECTOR_T* m, int n)
{
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "i(%f,%f,%f,%f)",m[0],m[1],m[2],m[3]);
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "i(%f,%f,%f,%f)",m[4],m[5],m[6],m[7]);
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "i(%f,%f,%f,%f)",m[8],m[9],m[10],m[11]);
    for(int i = 0; i < n; i++)
    {
        int max = i;
        for(int j = i+1; j < n; j++)
        {
            //find max m[xxx][j]...
            if(fabs(m[j*(n+1)+i]) > fabs(m[max*(n+1)+i]))
            {
                max = j;
            }
        }
        if(max != i)
        {
            //swap line max and line i
            ROTATION_VECTOR_T t;
            for(int j = i; j < n+1; j++)
            {
                t = m[i*(n+1)+j];
                m[i*(n+1)+j] = m[max*(n+1)+j];
                m[max*(n+1)+j] = t;
            }
        }
        if(fabs(m[i*(n+2)]) < 0.001f)
        {
            //must construct another matrix...
            TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]ERROR!");
            return -1;
        }
        ROTATION_VECTOR_T temp = m[i*(n+2)];
        for(int j = i; j < n+1; j++)
        {
            //unit m[i][i]
            m[i*(n+1)+j] /= temp;
        }
        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "%d(%f,%f,%f,%f)",i,m[0],m[1],m[2],m[3]);
        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "%d(%f,%f,%f,%f)",i,m[4],m[5],m[6],m[7]);
        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "%d(%f,%f,%f,%f)",i,m[8],m[9],m[10],m[11]);

        for(int j = i+1; j < n; j++)
        {
            //m[j][k] =m[j][k]/m[j][i] - m[i][k];
            temp = m[j*(n+1)+i];
            for(int k = i; k < n+1; k++)
            {
                m[j*(n+1)+k] = m[j*(n+1)+k]/temp - m[i*(n+1)+k];
            }
            m[j*(n+1)+i] = .0f;
        }
        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "%d(%f,%f,%f,%f)",i,m[0],m[1],m[2],m[3]);
        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "%d(%f,%f,%f,%f)",i,m[4],m[5],m[6],m[7]);
        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "%d(%f,%f,%f,%f)",i,m[8],m[9],m[10],m[11]);
    }
    for(int i = n-2; i >=0; i--)
    {
        for(int j = i; j >= 0; j--)
        {
            //m[j][n]-=m[i+1][n]*m[j][i]
            //m[j][i+1] = 0
            m[j*(n+1)+n] -= m[(i+1)*(n+1)+n]*m[j*(n+1)+i+1];
            m[j*(n+1)+i+1] = .0f;
        }
    }
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "o(%f,%f,%f,%f)",m[0],m[1],m[2],m[3]);
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "o(%f,%f,%f,%f)",m[4],m[5],m[6],m[7]);
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "o(%f,%f,%f,%f)",m[8],m[9],m[10],m[11]);
    return 0;
}

static void IncreaseMatrix(ROTATION_VECTOR_T* const m, const ROTATION_VECTOR_T* const magn)
{
    float x=magn[0],y=magn[1],z=magn[2];// we use raw data to calcuate offset, so no need +cali->magParams3D.b
    float w=x*x+y*y+z*z;
    m[0]+=x*x;m[1]+=x*y;m[2]+=x*z;m[3]+=x;m[4]+=x*w;
    m[5]=m[1];m[6]+=y*y;m[7]+=y*z;m[8]+=y;m[9]+=y*w;
    m[10]+=x*z;m[11]=m[7];m[12]+=z*z;m[13]+=z;m[14]+=z*w;
    m[15]+=x;m[16]+=y;m[17]+=z;m[18]++;m[19]+=w;
}

static void DecreaseMatrix(ROTATION_VECTOR_T* const m, const ROTATION_VECTOR_T* const magn)
{
    float x=magn[0],y=magn[1],z=magn[2];// we use raw data to calcuate offset, so no need +cali->magParams3D.b
    float w=x*x+y*y+z*z;
    m[0]-=x*x;m[1]-=x*y;m[2]-=x*z;m[3]-=x;m[4]-=x*w;
    m[5]=m[1];m[6]-=y*y;m[7]-=y*z;m[8]-=y;m[9]-=y*w;
    m[10]-=x*z;m[11]=m[7];m[12]-=z*z;m[13]-=z;m[14]-=z*w;
    m[15]-=x;m[16]-=y;m[17]-=z;m[18]--;m[19]-=w;
}

static int EstimateError(IN PMAGN_DYNAMICCALI_T cali, IN const ROTATION_VECTOR_T* const mag)
{
    //calcuate latest 6 variance to estimate...
    int end = cali->ErrorFifoEnd;
    int samples = cali->ErrorFifoSamples;
    float* fifo = cali->ErrorFifo;
    float offset[NUM_AXES] = {cali->magParams3D.b.m[AXIS_X], cali->magParams3D.b.m[AXIS_Y], cali->magParams3D.b.m[AXIS_Z]};

    float r[6];
    float ra = .0f; /*radius average*/
    float variance = .0f;

    //do we need update to fifo?
    float x0=mag[0],y0=mag[1],z0=mag[2];
    for(int i = 0; i < samples; i++)
    {
        float xi = fifo[i*3], yi= fifo[i*3+1], zi = fifo[i*3+2];
        float cos = (x0*xi+y0*yi+z0*zi)/sqrt((x0*x0+y0*y0+z0*z0)*(xi*xi+yi*yi+zi*zi));
        if(cos>.966f/*cos(15degree)*/)
            return -1;
    }
    //update fifo
    fifo[end*3] = mag[0];
    fifo[end*3+1] = mag[1];
    fifo[end*3+2] = mag[2];
    BackupRegisterWrite((BACKUP_REGISTER_T)(BKP_ERROR_FIFO_X0+end*3), (uint16_t)(mag[0]));
    BackupRegisterWrite((BACKUP_REGISTER_T)(BKP_ERROR_FIFO_X0+end*3+1), (uint16_t)(mag[1]));
    BackupRegisterWrite((BACKUP_REGISTER_T)(BKP_ERROR_FIFO_X0+end*3+2), (uint16_t)(mag[2]));

    end = end >=5?0:end+1;
    BackupRegisterWrite(BKP_ERROR_FIFO_END, (uint16_t)(end));
    cali->ErrorFifoEnd = end;

    if(samples < 6)
    {
        samples ++;
        BackupRegisterWrite(BKP_ERROR_FIFO_SAMPLES, (uint16_t)(samples));
        cali->ErrorFifoSamples = samples;
        return 180;/*return maxium value*/
    }

    for(int i = 0; i< samples; i++)
    {
        float x = fifo[i*3]-offset[AXIS_X], y= fifo[i*3+1]-offset[AXIS_Y], z = fifo[i*3+2]-offset[AXIS_Z];
        r[i] = sqrt(x*x+y*y+z*z);
        ra += r[i];
    }
    ra/=samples;
    for(int i = 0; i< samples; i++)
    {
        variance += (r[i]-ra)*(r[i]-ra);
    }
    variance/=samples;
    int estimate = (int)((2*RAD_TO_DEG*sqrt(variance)/ra)+.5f);
    cali->Error = estimate;
    BackupRegisterWrite(BKP_ERROR_FIFO_ERROR, (uint16_t)(estimate));
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]variance=%f,EstimateError=%d",variance,estimate);
    return estimate;
}

//see document for reference...
void RefineMagOffsetsWithGyro(IN PMAGN_DYNAMICCALI_T cali, IN const ROTATION_VECTOR_T* const mag, IN const ROTATION_VECTOR_T* const gyro, IN const float magConfidence)
{
    UNREFERENCED_PARAMETER(magConfidence);
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]
    float current_mag[]={mag[0],mag[1],mag[2]};
    float current_gyro[]={fabs(gyro[0]/1000),fabs(gyro[1]/1000),fabs(gyro[2]/1000)};//convert to degree/second
    float* m = cali->EqualizationArray;
    // Only run offset refinement if the gyro norm is larger than some threshold
    const float MIN_OFFSET_REFINEMENT_GYRO_NORM = 5.0f;
    const float MAX_OFFSET_REFINEMENT_GYRO_NORM = 90.0f;

    int *number = cali->FifoSize;
    unsigned *samples = cali->FifoSamples;

    BOOL valid_gyro[3]={current_gyro[0]>MIN_OFFSET_REFINEMENT_GYRO_NORM &&current_gyro[0]<MAX_OFFSET_REFINEMENT_GYRO_NORM,
        current_gyro[1]>MIN_OFFSET_REFINEMENT_GYRO_NORM &&current_gyro[1]<MAX_OFFSET_REFINEMENT_GYRO_NORM,
        current_gyro[2]>MIN_OFFSET_REFINEMENT_GYRO_NORM &&current_gyro[2]<MAX_OFFSET_REFINEMENT_GYRO_NORM};

    //calcuate store priority ===>p[3]
    int p0=samples[0]<=samples[1]?0:1; //use <= to avoid p0==p2
    p0=samples[p0]<=samples[2]?p0:2;
    int p2=samples[0]>samples[1]?0:1;
    p2=samples[p2]>samples[2]?p2:2;
    int p1=(~p0^p2)&3;

    int p[3]={p0,p1,p2};
    for(int i=0;i<3;i++)
    {
        int index = p[i];
        if(valid_gyro[index])
        {
            float* fifo = cali->Fifo[index];
            int position = cali->FifoIndex[index];
            if(number[index]>100-1/*99*/)
            {
                //swap out the first one
                DecreaseMatrix(m, &fifo[3*position]);
                number[index]--;
            }

            fifo[3*position] = current_mag[0];// we use raw data to calcuate offset, so no need +cali->magParams3D.b
            fifo[3*position+1] = current_mag[1];
            fifo[3*position+2] = current_mag[2];

            IncreaseMatrix(m, &fifo[3*position]);
            number[index]++;
            position = position>100-1/*99*/?0:position+1;
            cali->FifoIndex[index] = position;
            samples[index]++; //will overflow in 6.8years, so no need consider overflow...

            float mm[20];
            for(int i=0;i<20;i++) mm[i]=m[i];
            if(CalcuateOffset(mm,4) == 0)
            {
                cali->magParams3D.b.m[AXIS_X] = mm[4]/2;
                cali->magParams3D.b.m[AXIS_Y] = mm[9]/2;
                cali->magParams3D.b.m[AXIS_Z] = mm[14]/2;
                // Store the new offsets in the backup registers
                BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_X_A, BKP_CALIBRATION_CENTROID_X_B, cali->magParams3D.b.m[AXIS_X]);
                BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_Y_A, BKP_CALIBRATION_CENTROID_Y_B, cali->magParams3D.b.m[AXIS_Y]);
                BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_Z_A, BKP_CALIBRATION_CENTROID_Z_B, cali->magParams3D.b.m[AXIS_Z]);
                TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]magn-offset(%d,%d,%d)",(int32_t)(cali->magParams3D.b.m[0]),(int32_t)(cali->magParams3D.b.m[1]),(int32_t)(cali->magParams3D.b.m[2]));
            }
            int estimate = EstimateError(cali, current_mag);
            if(estimate<0)
            {
                TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]magn-offset(%d,%d,%d)",(int32_t)(cali->magParams3D.b.m[0]),(int32_t)(cali->magParams3D.b.m[1]),(int32_t)(cali->magParams3D.b.m[2]));
            }
            else
            {
                cali->Error = estimate;
            }
            break;
        }
    }
}

//see document for reference...
void RefineMagOffsets(IN PMAGN_DYNAMICCALI_T cali, IN const ROTATION_VECTOR_T* const mag, IN const float magConfidence)
{
    UNREFERENCED_PARAMETER(magConfidence);

    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]
//...cjy     static float last_mag[]={mag[0],mag[1],mag[2]};
    static float last_mag[]={.0f,1.f,.0f};
    float current_mag[]={mag[0],mag[1],mag[2]};
    float current_gyro[]={fabs((mag[2]-last_mag[2])*(mag[1]-last_mag[1]))/20,fabs((mag[2]-last_mag[2])*(mag[0]-last_mag[0]))/20,fabs((mag[0]-last_mag[0])*(mag[1]-last_mag[1]))/20};//convert to degree/second
    last_mag[0]=mag[0];
    last_mag[1]=mag[1];
    last_mag[2]=mag[2];

    float* m = cali->EqualizationArray;
    // Only run offset refinement if the gyro norm is larger than some threshold
    const float MIN_OFFSET_REFINEMENT_GYRO_NORM = 30.0f;
    const float MAX_OFFSET_REFINEMENT_GYRO_NORM = 180.0f;

    int *number = cali->FifoSize;
    unsigned *samples = cali->FifoSamples;

    BOOL valid_gyro[3]={current_gyro[0]>MIN_OFFSET_REFINEMENT_GYRO_NORM &&current_gyro[0]<MAX_OFFSET_REFINEMENT_GYRO_NORM,
        current_gyro[1]>MIN_OFFSET_REFINEMENT_GYRO_NORM &&current_gyro[1]<MAX_OFFSET_REFINEMENT_GYRO_NORM,
        current_gyro[2]>MIN_OFFSET_REFINEMENT_GYRO_NORM &&current_gyro[2]<MAX_OFFSET_REFINEMENT_GYRO_NORM};

    //calcuate store priority ===>p[3]
    int p0=samples[0]<=samples[1]?0:1; //use <= to avoid p0==p2
    p0=samples[p0]<=samples[2]?p0:2;
    int p2=samples[0]>samples[1]?0:1;
    p2=samples[p2]>samples[2]?p2:2;
    int p1=(~p0^p2)&3;

    int p[3]={p0,p1,p2};
    for(int i=0;i<3;i++)
    {
        int index = p[i];
        if(valid_gyro[index])
        {
            float* fifo = cali->Fifo[index];
            int position = cali->FifoIndex[index];
            if(number[index]>100-1/*99*/)
            {
                //swap out the first one
                DecreaseMatrix(m, &fifo[3*position]);
                number[index]--;
            }

            fifo[3*position] = current_mag[0];// we use raw data to calcuate offset, so no need +cali->magParams3D.b
            fifo[3*position+1] = current_mag[1];
            fifo[3*position+2] = current_mag[2];

            IncreaseMatrix(m, &fifo[3*position]);
            number[index]++;
            position = position>100-1/*99*/?0:position+1;
            cali->FifoIndex[index] = position;
            samples[index]++; //will overflow in 6.8years, so no need consider overflow...

            float mm[20];
            for(int i=0;i<20;i++) mm[i]=m[i];
            if(CalcuateOffset(mm,4) == 0)
            {
                cali->magParams3D.b.m[AXIS_X] = mm[4]/2;
                cali->magParams3D.b.m[AXIS_Y] = mm[9]/2;
                cali->magParams3D.b.m[AXIS_Z] = mm[14]/2;
                // Store the new offsets in the backup registers
                BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_X_A, BKP_CALIBRATION_CENTROID_X_B, cali->magParams3D.b.m[AXIS_X]);
                BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_Y_A, BKP_CALIBRATION_CENTROID_Y_B, cali->magParams3D.b.m[AXIS_Y]);
                BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_Z_A, BKP_CALIBRATION_CENTROID_Z_B, cali->magParams3D.b.m[AXIS_Z]);
                TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]magn-offset(%d,%d,%d)",(int32_t)(cali->magParams3D.b.m[0]),(int32_t)(cali->magParams3D.b.m[1]),(int32_t)(cali->magParams3D.b.m[2]));
            }
            int estimate = EstimateError(cali, current_mag);
            if(estimate<0)
            {
                TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]magn-offset(%d,%d,%d)",(int32_t)(cali->magParams3D.b.m[0]),(int32_t)(cali->magParams3D.b.m[1]),(int32_t)(cali->magParams3D.b.m[2]));
            }
            else
            {
                cali->Error = estimate;
            }
            break;
        }
    }
}

static BOOLEAN CentroidIsValid(IN const float centroidX,
                               IN const float centroidY,
                               IN const float centroidZ)
{
    // Good centroids should be close to zero since the centroid is applied 
    // after soft iron scaling.
    // Choose a conservative MAX_CENTROID_ABS of 8.0 since we've never seen it get above 4.0.
    const float MAX_CENTROID_ABS = 400.0f;
    if ((fabs(centroidX) > MAX_CENTROID_ABS) ||
        (fabs(centroidY) > MAX_CENTROID_ABS) ||
        (fabs(centroidZ) > MAX_CENTROID_ABS) ||
        (FALSE == isfinite(centroidX) ||
        FALSE == isfinite(centroidY) ||
        FALSE == isfinite(centroidZ)))
    {
        return FALSE;
    }   

    return TRUE;
}

void InitializeMagDynamic(INOUT PMAGN_DYNAMICCALI_T cali, IN pIvhPlatform platform)
{
    cali->calibrationState = CALIBRATION_STATE_INIT; 

    cali->magParams3D.a.rows = 3;
    cali->magParams3D.a.cols = 3;
    SAFE_MEMSET(cali->magParams3D.a.m, 0);
    cali->magParams3D.b.rows = 3;
    cali->magParams3D.b.cols = 1;
    SAFE_MEMSET(cali->magParams3D.b.m, 0);

    // Current anomaly state
    cali->inAnomaly = FALSE;

    cali->PLANE_NONE = 0x00;
    cali->PLANE_XY   = 0x01;
    cali->PLANE_XZ   = 0x02;
    cali->PLANE_YZ   = 0x04;

    cali->anomalyHoldCount = 0;

    //for RefineMagOffsets
    cali->prevMagNorm = 0;

    SAFE_MEMSET(cali->s_prevCalibratedMag, 0);
    SAFE_MEMSET(cali->s_prevRotatedMag, 0);

    SAFE_MEMSET(cali->rmat, 0);
    cali->rmat[0] = 1.0f;
    cali->rmat[4] = 1.0f;
    cali->rmat[8] = 1.0f;

    cali->lastNorm = .0f;
    cali->lastTime = 0;
    cali->sampleCount = 0;
    cali->lastAnomalyState = FALSE;
    cali->averageNorm = .0f;
    cali->prevNorm = 1.0;

    cali->Platform = platform;
    platform->Calibrate = cali;
    //new data struct
    SAFE_ZERO_MEM(&cali->EqualizationArray, sizeof(cali->EqualizationArray));
    SIZE_T size = sizeof(ROTATION_VECTOR_T)*4/*every sample have 4 elements*/*100/*fifo size is 100*/*3/*x,y,z*/;
    void* memory = SAFE_ALLOCATE_POOL(NonPagedPool, size, IVH_DYNAMIC_CALIBRATION_POOL_TAG);

    if (memory) 
    {
        // ZERO it!
        SAFE_FILL_MEM (memory, size, 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for magnetic dynamic calibration");
    }
    else 
    {
        //  No memory
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "not enough memory");
        return;
    }
    cali->Fifo[0] = (ROTATION_VECTOR_T*)(memory);
    cali->Fifo[1] = (ROTATION_VECTOR_T*)(memory)+100*4;
    cali->Fifo[2] = (ROTATION_VECTOR_T*)(memory)+200*4;
    SAFE_ZERO_MEM(&cali->FifoIndex, sizeof(cali->FifoIndex));
    SAFE_ZERO_MEM(&cali->FifoSize, sizeof(cali->FifoSize));
    SAFE_ZERO_MEM(&cali->FifoSamples, sizeof(cali->FifoSamples));
}

void UnInitializeMagDynamic(IN PMAGN_DYNAMICCALI_T cali)
{
    void* memory = cali->Fifo[0];
    if(memory != NULL)
    {
        SAFE_FREE_POOL(memory, IVH_DYNAMIC_CALIBRATION_POOL_TAG);
        cali->Fifo[0] = NULL;
        cali->Fifo[1] = NULL;
        cali->Fifo[2] = NULL;
    }
}

static void InitializeMagDynamicParams(INOUT PMAGN_DYNAMICCALI_T cali, IN pIvhPlatform platform)
{
    PPLATFORM_CALIBRATION_T calibration = &platform->PlatformCalibration;

    // Initialize the magParams with values from platformCalibration
    CalibrationCopyCorrectionMatrix(&(calibration->mag3D.correctionMatrix),
        &(cali->magParams3D.a),
        &(cali->magParams3D.b));

    // Check if all the backup registers are uninitialized
    BOOLEAN centroidBackupIsInitialized = TRUE;
    if ((BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_X_A)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_X_B)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_Y_A)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_Y_B)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_Z_A)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_Z_B)))
    {
        centroidBackupIsInitialized = FALSE;
    }   

    // If initialized, is the centroid valid?
    if (TRUE == centroidBackupIsInitialized)
    {
        float bkpCentroidX, bkpCentroidY, bkpCentroidZ;
        BackupRegisterReadFloat(BKP_CALIBRATION_CENTROID_X_A, BKP_CALIBRATION_CENTROID_X_B,
            &bkpCentroidX);
        BackupRegisterReadFloat(BKP_CALIBRATION_CENTROID_Y_A, BKP_CALIBRATION_CENTROID_Y_B,
            &bkpCentroidY);
        BackupRegisterReadFloat(BKP_CALIBRATION_CENTROID_Z_A, BKP_CALIBRATION_CENTROID_Z_B,
            &bkpCentroidZ);

        if (TRUE == CentroidIsValid(bkpCentroidX, bkpCentroidY, bkpCentroidZ))
        {
            // Overwrite the mag params in memory with values from the backup registers
            cali->magParams3D.b.m[AXIS_X] = bkpCentroidX;
            cali->magParams3D.b.m[AXIS_Y] = bkpCentroidY;
            cali->magParams3D.b.m[AXIS_Z] = bkpCentroidZ;
        }

        cali->ErrorFifoHead = (int)(BackupRegisterRead(BKP_ERROR_FIFO_HEAD));
        cali->ErrorFifoEnd = (int)(BackupRegisterRead(BKP_ERROR_FIFO_END));
        cali->ErrorFifoSamples = (int)(BackupRegisterRead(BKP_ERROR_FIFO_SAMPLES));
        int FifoReg = BKP_ERROR_FIFO_X0;
        for(int i = 0; i < 6*3; i++)
        {
            cali->ErrorFifo[i] = (ROTATION_VECTOR_T)(BackupRegisterRead((BACKUP_REGISTER_T)(FifoReg++)));
        }
        cali->Error = (int)(BackupRegisterRead(BKP_ERROR_FIFO_ERROR));
    }

    // Store the initialized mag params in the backup registers.
    BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_X_A, BKP_CALIBRATION_CENTROID_X_B,
        cali->magParams3D.b.m[AXIS_X]);
    BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_Y_A, BKP_CALIBRATION_CENTROID_Y_B,
        cali->magParams3D.b.m[AXIS_Y]);
    BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_Z_A, BKP_CALIBRATION_CENTROID_Z_B,
        cali->magParams3D.b.m[AXIS_Z]);                

    cali->correctionAtCardinals[ZUP][NORTH]    = calibration->headingCorrection.correctionZUp.correctionN;
    cali->correctionAtCardinals[ZUP][EAST]     = calibration->headingCorrection.correctionZUp.correctionE;
    cali->correctionAtCardinals[ZUP][SOUTH]    = calibration->headingCorrection.correctionZUp.correctionS;
    cali->correctionAtCardinals[ZUP][WEST]     = calibration->headingCorrection.correctionZUp.correctionW;

    // The sense of the offset correction is reversed when upside down.
    // The offsets are still specified in a human friendly way, but are reversed
    // here in initialization.
    cali->correctionAtCardinals[ZDN][NORTH]    = -calibration->headingCorrection.correctionZDn.correctionN;
    cali->correctionAtCardinals[ZDN][EAST]     = -calibration->headingCorrection.correctionZDn.correctionE;
    cali->correctionAtCardinals[ZDN][SOUTH]    = -calibration->headingCorrection.correctionZDn.correctionS;
    cali->correctionAtCardinals[ZDN][WEST]     = -calibration->headingCorrection.correctionZDn.correctionW;

    // Do something sensible in case the DFU is an older version that doesn't support the heading offsets
    // or if we somehow read garbage from the DFU
    int8_t i,j;
    const uint16_t MAX_HEADING_CORRECTION_ABS = 90;
    for (i=0; i<NUM_ORIENTATIONS; ++i)
    {
        for (j=0; j<NUM_CARDINALS; ++j)
        {
            if (MAX_HEADING_CORRECTION_ABS < abs(cali->correctionAtCardinals[i][j]))
            {
                cali->correctionAtCardinals[i][j] = 0;
            }
        }
    }
}

static void RotateMagPoints(IN const float degreesToRotate,
                            INOUT ROTATION_VECTOR_T mag[NUM_AXES])
{
    MATRIX_STRUCT_T mRotatedResult = {3, 1, {.0f, .0f, .0f}};
    MATRIX_STRUCT_T mMag = {3, 1, {(float)(mag[AXIS_X]),
        (float)(mag[AXIS_Y]),
        (float)(mag[AXIS_Z])}};
    const float radsToRotate = degreesToRotate * DEG_TO_RAD;
    const MATRIX_STRUCT_T mRotate = {3, 3, {(float)(cosf(radsToRotate)), -(float)(sinf(radsToRotate)), .0f,
        (float)(sinf(radsToRotate)),  (float)(cosf(radsToRotate)), .0f,
        .0f,                                 .0f,  1.0f}};
    MatrixMultiply(&mRotate, &mMag, &mRotatedResult);

    mag[AXIS_X] = mRotatedResult.m[AXIS_X];
    mag[AXIS_Y] = mRotatedResult.m[AXIS_Y];
    mag[AXIS_Z] = mRotatedResult.m[AXIS_Z];
}

static float CalculateMagCorrection(IN PMAGN_DYNAMICCALI_T cali,
                                    IN const float heading,
                                    IN const float pitch,
                                    IN const float roll)
{
    const int16_t HEADING_NORTH = 0;
    const int16_t HEADING_EAST = 90;
    const int16_t HEADING_SOUTH = 180;
    const int16_t HEADING_WEST = 270;
    const int16_t headingAtCardinals[NUM_CARDINALS] = {HEADING_NORTH, HEADING_EAST, HEADING_SOUTH, HEADING_WEST};

    int8_t leftCardinal, rightCardinal;
    if (heading >= HEADING_NORTH && heading < HEADING_EAST)
    {
        leftCardinal = NORTH;
        rightCardinal = EAST;
    }
    else
    {
        if (heading >= HEADING_EAST && heading < HEADING_SOUTH)
        {
            leftCardinal = EAST;
            rightCardinal = SOUTH;
        }
        else
        {
            if (heading >= HEADING_SOUTH && heading < HEADING_WEST)
            {
                leftCardinal = SOUTH;
                rightCardinal = WEST;
            }
            else
            {
                leftCardinal = WEST;
                rightCardinal = NORTH;
            }
        }
    }

    const int16_t DEGREES_BETWEEN_CARDINALS = 90;
    float percentOfRightCardinal = ((heading - headingAtCardinals[leftCardinal]) / DEGREES_BETWEEN_CARDINALS);
    float percentOfLeftCardinal = 1 - percentOfRightCardinal;

    float correction;
    // Apply different correction for rightside up vs upside-down
    if (fabs(pitch) < 90)
    {
        // Pitch from 0 to +-90 is right side up
        correction = (percentOfRightCardinal  * cali->correctionAtCardinals[ZUP][rightCardinal]) + 
            (percentOfLeftCardinal * cali->correctionAtCardinals[ZUP][leftCardinal]);
    }
    else
    {
        // Pitch from +-90 to 180 is upside down
        correction = (percentOfRightCardinal  * cali->correctionAtCardinals[ZDN][rightCardinal]) + 
            (percentOfLeftCardinal * cali->correctionAtCardinals[ZDN][leftCardinal]);
    }

    // reduce correction as pitch nears +-90 degrees
    float pitch0to90 = MIN(fabs(pitch), fabs(fabs(pitch)-180));
    float pitchPercentWeight = 1 - (pitch0to90 / 90);
    correction *= pitchPercentWeight;

    float rollPercentWeight = 1 - (fabs(roll) / 90);
    correction *= rollPercentWeight;

    return ConversionNormalizeDegrees(correction);
}

// The main entry point for dynamic adaptation.  
// This function is responsible for converting rawMag into calibratedMag.
void MagDynamicCali(IN PMAGN_DYNAMICCALI_T cali,
                    INOUT IvhSensorData* const pSensorData,
                    IN const IvhSensorData* const pAccelData)
{    
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "%!FUNC! Entry");
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "accl(%d,%d,%d),mag(%d,%d,%d)",pAccelData->data.accel.xyz.x,pAccelData->data.accel.xyz.y,pAccelData->data.accel.xyz.z,
        pSensorData->data.mag.xyzRaw.x,pSensorData->data.mag.xyzRaw.y,pSensorData->data.mag.xyzRaw.z);

    if (CALIBRATION_STATE_INIT == cali->calibrationState)
    {
        InitializeMagDynamicParams(cali, cali->Platform);
        cali->calibrationState = CALIBRATION_STATE_RUNNING;
    }
    else
    {
        // Sanity check the current mag calibration params and restore from backup registers
        // if they are bad.  If the backup registers are also 
        if (FALSE == CentroidIsValid(cali->magParams3D.b.m[AXIS_X],
            cali->magParams3D.b.m[AXIS_Y],
            cali->magParams3D.b.m[AXIS_Z]))                                 
        {
            CalibrationCopyCorrectionMatrix(&(cali->Platform->PlatformCalibration.mag3D.correctionMatrix),
                &(cali->magParams3D.a),
                &(cali->magParams3D.b));
        }
    }

    MATRIX_STRUCT_T mResult = {3, 1, {.0f, .0f, .0f}};                               
    MATRIX_STRUCT_T mMag = {3, 1, {(float)(pSensorData->data.mag.xyzRaw.x),
        (float)(pSensorData->data.mag.xyzRaw.y),
        (float)(pSensorData->data.mag.xyzRaw.z)}};

    // Shift the point using the hard iron offsets
    mMag.m[AXIS_X] -= cali->magParams3D.b.m[AXIS_X];
    mMag.m[AXIS_Y] -= cali->magParams3D.b.m[AXIS_Y];
    mMag.m[AXIS_Z] -= cali->magParams3D.b.m[AXIS_Z]; 

    // Apply soft iron correction using a 3D rotation and scaling matrix
    //TODO: soft iron correction can compensate device mount error and soft iron interference
    //the matrix will be calculated by calibration tool and set to system non-violate memory
    MatrixMultiply(&(cali->magParams3D.a), &mMag, &mResult);

    pSensorData->data.mag.xyzCalibrated.x = mResult.m[AXIS_X];
    pSensorData->data.mag.xyzCalibrated.y = mResult.m[AXIS_Y];
    pSensorData->data.mag.xyzCalibrated.z = mResult.m[AXIS_Z];    

    ROTATION_VECTOR_T magVector[NUM_AXES] = {pSensorData->data.mag.xyzCalibrated.x,
        pSensorData->data.mag.xyzCalibrated.y,
        pSensorData->data.mag.xyzCalibrated.z};
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "mag.cali(%f,%f,%f)",magVector[0],magVector[1],magVector[2]);

    // this compensation mothed is a compensation of soft iron compensation especially when soft iron interference is significant
    // TODO : the data will be calculated by calibration tool and set to system non-violate memory

    // On HSB there is some rotation of the raw data that is not corrected with the soft iron
    // correction matrix.  To get through WHCK testing, apply a rotation here to the calibrated data.
    // The calibrated data at this point should be on a circle and centered around (0, 0, 0)

    ROTATION_VECTOR_T accelVector[NUM_AXES] = {(ROTATION_DATA_T)(pAccelData->data.accel.xyz.x),
        (ROTATION_DATA_T)(pAccelData->data.accel.xyz.y),
        (ROTATION_DATA_T)(pAccelData->data.accel.xyz.z)};

    float yaw, pitch, roll;
    CalculateRotationUsingAccel(accelVector, magVector, cali->rmat);
    ConvertRotationToEulerAngles(cali->rmat, &yaw, &pitch, &roll);

    float heading = ConversionNormalizeDegrees(360 - yaw);
    // Because of the pitch, roll, yaw order, the heading is 
    // 360 when pointing south, and 180 when pointing north,
    // decreasing when rotating clockwise.  We need to normalize the
    // heading so that it make sense when using the device
    if (fabs(pitch) > 90)
    {
        heading = ConversionNormalizeDegrees(180 - heading);
    }

    float correction = CalculateMagCorrection(cali, heading, pitch, roll);

    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "correction=%f",correction);
    RotateMagPoints(correction, magVector);
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "mag.rot(%f,%f,%f)",magVector[0],magVector[1],magVector[2]);

    pSensorData->data.mag.xyzRotated.x = magVector[AXIS_X];
    pSensorData->data.mag.xyzRotated.y = magVector[AXIS_Y];
    pSensorData->data.mag.xyzRotated.z = magVector[AXIS_Z]; 

    // Apply a low pass filter to avoid wild swings back and forth
    if (cali->Platform->CalibrationFeatures.enableMagnetometerLowPassFilter)
    {
        ROTATION_VECTOR_T calibratedMag[NUM_AXES] = {pSensorData->data.mag.xyzCalibrated.x,
            pSensorData->data.mag.xyzCalibrated.y,
            pSensorData->data.mag.xyzCalibrated.z};
        ApplyLowPassFilter(calibratedMag, 
            cali->s_prevCalibratedMag, 
            (sizeof(cali->s_prevCalibratedMag) / sizeof(cali->s_prevCalibratedMag[0])), 
            cali->Platform->CalibrationSettings.magLowPassFilterAlpha);

        pSensorData->data.mag.xyzCalibrated.x = calibratedMag[AXIS_X];
        pSensorData->data.mag.xyzCalibrated.y = calibratedMag[AXIS_Y];
        pSensorData->data.mag.xyzCalibrated.z = calibratedMag[AXIS_Z];

        ROTATION_VECTOR_T rotatedMag[NUM_AXES] = {pSensorData->data.mag.xyzRotated.x,
            pSensorData->data.mag.xyzRotated.y,
            pSensorData->data.mag.xyzRotated.z};
        TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "mag.last(%f,%f,%f,%f)",cali->s_prevRotatedMag[0],cali->s_prevRotatedMag[1],cali->s_prevRotatedMag[2],cali->Platform->CalibrationSettings.magLowPassFilterAlpha);
        //TODO: must use adaptive filter here due to magn sample rate is too slow!!!
        ApplyLowPassFilter(rotatedMag, 
            cali->s_prevRotatedMag, 
            (sizeof(cali->s_prevRotatedMag) / sizeof(cali->s_prevRotatedMag[0])), 
            cali->Platform->CalibrationSettings.magLowPassFilterAlpha);

        pSensorData->data.mag.xyzRotated.x = rotatedMag[AXIS_X];
        pSensorData->data.mag.xyzRotated.y = rotatedMag[AXIS_Y];
        pSensorData->data.mag.xyzRotated.z = rotatedMag[AXIS_Z];
        TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "mag.lpf(%f,%f,%f)",rotatedMag[0],rotatedMag[1],rotatedMag[2]);
    }
}

// Watch differences in the trend of the magnetometer norm
// to calculate a magnetometer confidence number between 0 and 1
void MagDetectAnomalyWithGyro(IN PMAGN_DYNAMICCALI_T cali,
                      IN const ROTATION_VECTOR_T* const calibratedMag,
                      IN const ROTATION_VECTOR_T* const gyro,
                      OUT float* const magConfidence)
{   
    uint32_t thisTime = TmrGetMillisecondCounter()/100;

    // Reset the sample count if the anomaly state changed
    if (cali->lastAnomalyState != cali->inAnomaly)
    {
        cali->sampleCount = 0;
        cali->lastAnomalyState = cali->inAnomaly;
    }

    // We need to be moving to run the anomaly detection algo
    // Use the gyro norm as a measure of overall rotational velocity
    // We'll use a mag confidence of 0.75 to indicate that no anomaly exists,
    // but we're not moving fast enough, or we're moving too fast
    // to run the anomaly algo
    float normGyro = CalculateVectorNorm(gyro);
    const float MIN_NORM_GYRO_ANOMALY_DETECTION = 5000;
    const float MAX_NORM_GYRO_ANOMALY_DETECTION = 180000;
    if (normGyro < MIN_NORM_GYRO_ANOMALY_DETECTION ||
        normGyro > MAX_NORM_GYRO_ANOMALY_DETECTION)
    {
        const float MAG_CONFIDENCE_OUTSIDE_GYRO_MINMAX_NORM = 0.75;
        *magConfidence = MIN(*magConfidence, MAG_CONFIDENCE_OUTSIDE_GYRO_MINMAX_NORM);
    }
    else
    {
        // Get the norm of the magnetometer and use a LPF to approximate the average
        float thisNorm = CalculateVectorNorm(calibratedMag);

        const float NORM_LPF_ALPHA = 0.02f;
        cali->averageNorm = thisNorm;
        if(cali->prevNorm < 50.0f) //we suppose magn norm never bellow 50 mini-gauss;
        {
            cali->prevNorm = thisNorm;
        }
        ApplyLowPassFilter(&cali->averageNorm, &cali->prevNorm, 1, NORM_LPF_ALPHA);

        // Find the derivative of the sample-to-sample norm.  If the derivative spikes
        // it means that we're experiencing a shift in the mag field.
        float dNorm = fabs(thisNorm - cali->lastNorm) / (float)(thisTime - cali->lastTime);
        float diffNormFromAverage = fabs(cali->averageNorm - thisNorm);

        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "diffNormFromAverage=%f,cali->averageNorm=%f,thisNorm=%f,dNorm=%f,cali->anomalyHoldCount=%d,cali->inAnomaly=%d,*magConfidence=%f)",diffNormFromAverage,cali->averageNorm,thisNorm,dNorm,cali->anomalyHoldCount,cali->inAnomaly,*magConfidence);

        // If not in anomaly mode, see if anything strange is happening that might
        // indicate an anomaly is present.
        if (FALSE == cali->inAnomaly)
        {
            // Delay startup of anomaly detection for a few samples to allow the dNorm to stablize
            const uint16_t ANOMALY_STARTUP_SAMPLES = 100;
            if (cali->sampleCount > ANOMALY_STARTUP_SAMPLES)
            {
                // If the current norm is outside some percentage from the running
                // average or the dNorm has spiked, or the norm is outside
                // the expected range, flag an anomaly
                const float PERCENT_ALLOWED_DIFF = 0.10f;    
                const float MAX_ALLOWED_NORM = 2.0f*330;
                const float MIN_ALLOWED_NORM = 0.4f*330;
                if ((diffNormFromAverage > (cali->averageNorm * PERCENT_ALLOWED_DIFF)) ||
                    (thisNorm > MAX_ALLOWED_NORM) ||
                    (thisNorm < MIN_ALLOWED_NORM) ||
                    (dNorm > cali->Platform->CalibrationSettings.anomalyTriggerPoint))
                {
                    *magConfidence = 0;
                    SetAnomalyState(cali, TRUE);
                }
                else
                {
                    *magConfidence = 1.0;
                }
            }
        }
        else  // We are in the anomaly.  Can we come out?
        {  
            *magConfidence = .0f;

            // If the current norm is within some percentage of the
            // norm before we declared anomaly, increase the anomaly hold count.
            const uint32_t ANOMALY_HOLD_COUNT_REQUIRED = 100;
            const float PERCENT_ALLOWED_DIFF = 0.10f;    
            if ((diffNormFromAverage < (cali->averageNorm * PERCENT_ALLOWED_DIFF)) &&
                (dNorm < cali->Platform->CalibrationSettings.anomalyTriggerPoint))
            {
                ++cali->anomalyHoldCount;
            }
            else
            {
                // If the value ever spikes outside the allowed range from the last good
                // norm, reset the hold count
                cali->anomalyHoldCount = 0;
            }

            // If the anomaly hold count exceeds the required count, then
            // exit the anomaly
            if (cali->anomalyHoldCount > ANOMALY_HOLD_COUNT_REQUIRED)
            {
                SetAnomalyState(cali, FALSE);
            }
        }

        // Increase the sample count and store norm and time for the next iteration
        ++cali->sampleCount;
        cali->lastNorm = thisNorm;
        cali->lastTime = thisTime;
    }
}

// Watch differences in the trend of the magnetometer norm
// to calculate a magnetometer confidence number between 0 and 1
void MagDetectAnomaly(IN PMAGN_DYNAMICCALI_T cali,
                      IN const ROTATION_VECTOR_T* const calibratedMag,
                      OUT float* const magConfidence)
{   
    UNREFERENCED_PARAMETER(cali);
    UNREFERENCED_PARAMETER(calibratedMag);
    UNREFERENCED_PARAMETER(magConfidence);
    //TODO: need enable it...
#if 0
    uint32_t thisTime = TmrGetMillisecondCounter()/100;

    // Reset the sample count if the anomaly state changed
    if (cali->lastAnomalyState != cali->inAnomaly)
    {
        cali->sampleCount = 0;
        cali->lastAnomalyState = cali->inAnomaly;
    }


    // We need to be moving to run the anomaly detection algo
    // Use the gyro norm as a measure of overall rotational velocity
    // We'll use a mag confidence of 0.75 to indicate that no anomaly exists,
    // but we're not moving fast enough, or we're moving too fast
    // to run the anomaly algo
    float normGyro = CalculateVectorNorm(gyro);
    const float MIN_NORM_GYRO_ANOMALY_DETECTION = 5000;
    const float MAX_NORM_GYRO_ANOMALY_DETECTION = 180000;
    if (normGyro < MIN_NORM_GYRO_ANOMALY_DETECTION ||
        normGyro > MAX_NORM_GYRO_ANOMALY_DETECTION)
    {
        const float MAG_CONFIDENCE_OUTSIDE_GYRO_MINMAX_NORM = 0.75;
        *magConfidence = MIN(*magConfidence, MAG_CONFIDENCE_OUTSIDE_GYRO_MINMAX_NORM);
    }
    else
    {
        // Get the norm of the magnetometer and use a LPF to approximate the average
        float thisNorm = CalculateVectorNorm(calibratedMag);

        const float NORM_LPF_ALPHA = 0.02f;
        cali->averageNorm = thisNorm;
        if(cali->prevNorm < 50.0f) //we suppose magn norm never bellow 50 mini-gauss;
        {
            cali->prevNorm = thisNorm;
        }
        ApplyLowPassFilter(&cali->averageNorm, &cali->prevNorm, 1, NORM_LPF_ALPHA);

        // Find the derivative of the sample-to-sample norm.  If the derivative spikes
        // it means that we're experiencing a shift in the mag field.
        float dNorm = fabs(thisNorm - cali->lastNorm) / (float)(thisTime - cali->lastTime);
        float diffNormFromAverage = fabs(cali->averageNorm - thisNorm);

        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "diffNormFromAverage=%f,cali->averageNorm=%f,thisNorm=%f,dNorm=%f,cali->anomalyHoldCount=%d,cali->inAnomaly=%d,*magConfidence=%f)",diffNormFromAverage,cali->averageNorm,thisNorm,dNorm,cali->anomalyHoldCount,cali->inAnomaly,*magConfidence);

        // If not in anomaly mode, see if anything strange is happening that might
        // indicate an anomaly is present.
        if (FALSE == cali->inAnomaly)
        {
            // Delay startup of anomaly detection for a few samples to allow the dNorm to stablize
            const uint16_t ANOMALY_STARTUP_SAMPLES = 100;
            if (cali->sampleCount > ANOMALY_STARTUP_SAMPLES)
            {
                // If the current norm is outside some percentage from the running
                // average or the dNorm has spiked, or the norm is outside
                // the expected range, flag an anomaly
                const float PERCENT_ALLOWED_DIFF = 0.10f;    
                const float MAX_ALLOWED_NORM = 2.0f*330;
                const float MIN_ALLOWED_NORM = 0.4f*330;
                if ((diffNormFromAverage > (cali->averageNorm * PERCENT_ALLOWED_DIFF)) ||
                    (thisNorm > MAX_ALLOWED_NORM) ||
                    (thisNorm < MIN_ALLOWED_NORM) ||
                    (dNorm > cali->Platform->CalibrationSettings.anomalyTriggerPoint))
                {
                    *magConfidence = 0;
                    SetAnomalyState(cali, TRUE);
                }
                else
                {
                    *magConfidence = 1.0;
                }
            }
        }
        else  // We are in the anomaly.  Can we come out?
        {  
            *magConfidence = .0f;

            // If the current norm is within some percentage of the
            // norm before we declared anomaly, increase the anomaly hold count.
            const uint32_t ANOMALY_HOLD_COUNT_REQUIRED = 100;
            const float PERCENT_ALLOWED_DIFF = 0.10f;    
            if ((diffNormFromAverage < (cali->averageNorm * PERCENT_ALLOWED_DIFF)) &&
                (dNorm < cali->Platform->CalibrationSettings.anomalyTriggerPoint))
            {
                ++cali->anomalyHoldCount;
            }
            else
            {
                // If the value ever spikes outside the allowed range from the last good
                // norm, reset the hold count
                cali->anomalyHoldCount = 0;
            }

            // If the anomaly hold count exceeds the required count, then
            // exit the anomaly
            if (cali->anomalyHoldCount > ANOMALY_HOLD_COUNT_REQUIRED)
            {
                SetAnomalyState(cali, FALSE);
            }
        }

        // Increase the sample count and store norm and time for the next iteration
        ++cali->sampleCount;
        cali->lastNorm = thisNorm;
        cali->lastTime = thisTime;
    }
#endif
}
