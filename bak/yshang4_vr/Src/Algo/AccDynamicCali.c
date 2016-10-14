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
#include "AccDynamicCali.h"
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
            TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]ACC ERROR!");
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

static int EstimateError(IN PACCL_DYNAMICCALI_T cali, IN const ROTATION_VECTOR_T* const mag)
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
    BackupRegisterWrite((BACKUP_REGISTER_T)(BKP_ERROR_FIFO_ACC_X0+end*3), (uint16_t)(mag[0]));
    BackupRegisterWrite((BACKUP_REGISTER_T)(BKP_ERROR_FIFO_ACC_X0+end*3+1), (uint16_t)(mag[1]));
    BackupRegisterWrite((BACKUP_REGISTER_T)(BKP_ERROR_FIFO_ACC_X0+end*3+2), (uint16_t)(mag[2]));

    end = end >=5?0:end+1;
    BackupRegisterWrite(BKP_ERROR_FIFO_ACC_END, (uint16_t)(end));
    cali->ErrorFifoEnd = end;

    if(samples < 6)
    {
        samples ++;
        BackupRegisterWrite(BKP_ERROR_FIFO_ACC_SAMPLES, (uint16_t)(samples));
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
    BackupRegisterWrite(BKP_ERROR_FIFO_ACC_ERROR, (uint16_t)(estimate));
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]variance=%f,EstimateError=%d",variance,estimate);
    return estimate;
}

//see document for reference...
#define MOTION_STILL_TRD (40*40*3)
void RefineAccOffsets(IN PACCL_DYNAMICCALI_T cali, IN const IvhSensorData* const pSensorData)
{
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]
    float acc[]={(float)(pSensorData->data.accel.xyz.x),(float)(pSensorData->data.accel.xyz.y),(float)(pSensorData->data.accel.xyz.z)};

    //store current acc data to fifo...
    static int numbers = 0;
    static int stream_index = 0;
    static float stream_fifo[15][3];
    stream_fifo[stream_index][0] = acc[0],stream_fifo[stream_index][1] = acc[1],stream_fifo[stream_index][2] = acc[2];
    stream_index = stream_index == 14?0:stream_index+1;
    numbers++;

    if(numbers <= 15)
    {
        //return if samples is not enough
        return;
    }

    //check latest 5 points
    int c = (stream_index+12/*15-3*/)%15; //current point
    int l1 = (stream_index+11)%15; //left 1 point
    int l2 = (stream_index+10)%15; //left 2 point
    int r1 = (stream_index+13)%15; //right 1 point
    int r2 = (stream_index+14)%15; //right 2 point
    float d1 = (stream_fifo[c][0]-stream_fifo[l1][0])*(stream_fifo[c][0]-stream_fifo[l1][0])
                + (stream_fifo[c][1]-stream_fifo[l1][1])*(stream_fifo[c][1]-stream_fifo[l1][1])
                + (stream_fifo[c][2]-stream_fifo[l1][2])*(stream_fifo[c][2]-stream_fifo[l1][2]);
    float d2 = (stream_fifo[c][0]-stream_fifo[l2][0])*(stream_fifo[c][0]-stream_fifo[l2][0])
                + (stream_fifo[c][1]-stream_fifo[l2][1])*(stream_fifo[c][1]-stream_fifo[l2][1])
                + (stream_fifo[c][2]-stream_fifo[l2][2])*(stream_fifo[c][2]-stream_fifo[l2][2]);
    float d3 = (stream_fifo[c][0]-stream_fifo[r1][0])*(stream_fifo[c][0]-stream_fifo[r1][0])
                + (stream_fifo[c][1]-stream_fifo[r1][1])*(stream_fifo[c][1]-stream_fifo[r1][1])
                + (stream_fifo[c][2]-stream_fifo[r1][2])*(stream_fifo[c][2]-stream_fifo[r1][2]);
    float d4 = (stream_fifo[c][0]-stream_fifo[r2][0])*(stream_fifo[c][0]-stream_fifo[r2][0])
                + (stream_fifo[c][1]-stream_fifo[r2][1])*(stream_fifo[c][1]-stream_fifo[r2][1])
                + (stream_fifo[c][2]-stream_fifo[r2][2])*(stream_fifo[c][2]-stream_fifo[r2][2]);
    if(d1 > MOTION_STILL_TRD || d2 > MOTION_STILL_TRD || d3 > MOTION_STILL_TRD || d4 > MOTION_STILL_TRD)
    {
        //return if system is in motion state
        return;
    }

    //check whether it's valid for store
    float current_acc[]={stream_fifo[c][0],stream_fifo[c][1],stream_fifo[c][2]};
    int *number = cali->FifoSize;
    unsigned *samples = cali->FifoSamples;
    float* fifo = cali->Fifo[0];
    int position = cali->FifoIndex[0];
    for(int i = 0; i < number[0]; i++)
    {
        //calcuate diff-norm
        float d[] = {current_acc[0]-fifo[3*((position+99-i)%100)+0],current_acc[1]-fifo[3*((position+99-i)%100)+1],current_acc[2]-fifo[3*((position+99-i)%100)+2]};
        float dn = sqrt(d[0]*d[0]+d[1]*d[1]+d[2]*d[2]);
        if(dn<100.f)
        {
            //return if fifo have similar samples
            return;
        }
    }

    //store to fifo
    TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "[%!FUNC!]add acc[%d](%d,%d,%d)",position,(int32_t)(current_acc[0]),(int32_t)(current_acc[1]),(int32_t)(current_acc[2]));
    float* m = cali->EqualizationArray;
    if(number[0]>100-1/*99*/)
    {
        //swap out the first one
        DecreaseMatrix(m, &fifo[3*position]);
        number[0]--;
    }
    fifo[3*position] = current_acc[0];// we use raw data to calcuate offset
    fifo[3*position+1] = current_acc[1];
    fifo[3*position+2] = current_acc[2];

    IncreaseMatrix(m, &fifo[3*position]);
    number[0]++;
    position = position>100-1/*99*/?0:position+1;
    cali->FifoIndex[0] = position;
    samples[0]++; //will overflow in 6.8years, so no need consider overflow...
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]acc(%d,%d,%d)acc-offset(%d,%d,%d)",pSensorData->data.accel.xyz.x,pSensorData->data.accel.xyz.y,pSensorData->data.accel.xyz.z,(int32_t)(cali->magParams3D.b.m[0]),(int32_t)(cali->magParams3D.b.m[1]),(int32_t)(cali->magParams3D.b.m[2]));

    //re-calcuate offset
    float mm[20];
    for(int i=0;i<20;i++) mm[i]=m[i];
    if(CalcuateOffset(mm,4) == 0)
    {
        cali->magParams3D.b.m[AXIS_X] = -mm[4]/2;
        cali->magParams3D.b.m[AXIS_Y] = -mm[9]/2;
        cali->magParams3D.b.m[AXIS_Z] = -mm[14]/2;
        // Store the new offsets in the backup registers
        BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_ACC_X_A, BKP_CALIBRATION_CENTROID_ACC_X_B, cali->magParams3D.b.m[AXIS_X]);
        BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_ACC_Y_A, BKP_CALIBRATION_CENTROID_ACC_Y_B, cali->magParams3D.b.m[AXIS_Y]);
        BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_ACC_Z_A, BKP_CALIBRATION_CENTROID_ACC_Z_B, cali->magParams3D.b.m[AXIS_Z]);
    }

    //estimate error
    int estimate = EstimateError(cali, current_acc);
    if(estimate<0)
    {
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]estimate<0");
    }
    else
    {
        cali->Error = estimate;
    }
}

static BOOLEAN CentroidIsValid(IN const float centroidX,
                               IN const float centroidY,
                               IN const float centroidZ)
{
    // Good centroids should be close to zero since the centroid is applied 
    // after soft iron scaling.
    // Choose a conservative MAX_CENTROID_ABS of 8.0 since we've never seen it get above 4.0.
    const float MAX_CENTROID_ABS = 240.0f;
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

void InitializeAccDynamic(INOUT PACCL_DYNAMICCALI_T cali, IN pIvhPlatform platform)
{
    cali->calibrationState = ACC_CALIBRATION_STATE_INIT; 

    cali->magParams3D.a.rows = 3;
    cali->magParams3D.a.cols = 3;
    SAFE_MEMSET(cali->magParams3D.a.m, 0);
    cali->magParams3D.b.rows = 3;
    cali->magParams3D.b.cols = 1;
    SAFE_MEMSET(cali->magParams3D.b.m, 0);

    // Current anomaly state
    cali->inAnomaly = FALSE;

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

void UnInitializeAccDynamic(IN PACCL_DYNAMICCALI_T cali)
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

static void InitializeAccDynamicParams(INOUT PACCL_DYNAMICCALI_T cali, IN pIvhPlatform platform)
{
    PPLATFORM_CALIBRATION_T calibration = &platform->PlatformCalibration;

    // Initialize the magParams with values from platformCalibration
    CalibrationCopyCorrectionMatrix(&(calibration->protractor.correctionMatrix),
        &(cali->magParams3D.a),
        &(cali->magParams3D.b));

    // Check if all the backup registers are uninitialized
    BOOLEAN centroidBackupIsInitialized = TRUE;
    if ((BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_ACC_X_A)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_ACC_X_B)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_ACC_Y_A)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_ACC_Y_B)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_ACC_Z_A)) &&
        (BKP_UNINITIALIZED_VALUE == BackupRegisterRead(BKP_CALIBRATION_CENTROID_ACC_Z_B)))
    {
        centroidBackupIsInitialized = FALSE;
    }   

    // If initialized, is the centroid valid?
    if (TRUE == centroidBackupIsInitialized)
    {
        float bkpCentroidX, bkpCentroidY, bkpCentroidZ;
        BackupRegisterReadFloat(BKP_CALIBRATION_CENTROID_ACC_X_A, BKP_CALIBRATION_CENTROID_ACC_X_B,
            &bkpCentroidX);
        BackupRegisterReadFloat(BKP_CALIBRATION_CENTROID_ACC_Y_A, BKP_CALIBRATION_CENTROID_ACC_Y_B,
            &bkpCentroidY);
        BackupRegisterReadFloat(BKP_CALIBRATION_CENTROID_ACC_Z_A, BKP_CALIBRATION_CENTROID_ACC_Z_B,
            &bkpCentroidZ);

        if (TRUE == CentroidIsValid(bkpCentroidX, bkpCentroidY, bkpCentroidZ))
        {
            // Overwrite the mag params in memory with values from the backup registers
            cali->magParams3D.b.m[AXIS_X] = bkpCentroidX;
            cali->magParams3D.b.m[AXIS_Y] = bkpCentroidY;
            cali->magParams3D.b.m[AXIS_Z] = bkpCentroidZ;
        }

        cali->ErrorFifoHead = (int)(BackupRegisterRead(BKP_ERROR_FIFO_ACC_HEAD));
        cali->ErrorFifoEnd = (int)(BackupRegisterRead(BKP_ERROR_FIFO_ACC_END));
        cali->ErrorFifoSamples = (int)(BackupRegisterRead(BKP_ERROR_FIFO_ACC_SAMPLES));
        int FifoReg = BKP_ERROR_FIFO_ACC_X0;
        for(int i = 0; i < 6*3; i++)
        {
            cali->ErrorFifo[i] = (ROTATION_VECTOR_T)(BackupRegisterRead((BACKUP_REGISTER_T)(FifoReg++)));
        }
        cali->Error = (int)(BackupRegisterRead(BKP_ERROR_FIFO_ACC_ERROR));
    }

    // Store the initialized mag params in the backup registers.
    BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_ACC_X_A, BKP_CALIBRATION_CENTROID_ACC_X_B,
        cali->magParams3D.b.m[AXIS_X]);
    BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_ACC_Y_A, BKP_CALIBRATION_CENTROID_ACC_Y_B,
        cali->magParams3D.b.m[AXIS_Y]);
    BackupRegisterWriteFloat(BKP_CALIBRATION_CENTROID_ACC_Z_A, BKP_CALIBRATION_CENTROID_ACC_Z_B,
        cali->magParams3D.b.m[AXIS_Z]);                

}

// The main entry point for dynamic adaptation.  
// This function is responsible for converting rawMag into calibratedMag.
void AccDynamicCali(IN PACCL_DYNAMICCALI_T cali,
                    INOUT IvhSensorData* const pSensorData)
{    
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "%!FUNC! Entry");

    if (ACC_CALIBRATION_STATE_INIT == cali->calibrationState)
    {
        InitializeAccDynamicParams(cali, cali->Platform);
        cali->calibrationState = ACC_CALIBRATION_STATE_RUNNING;
    }
    else
    {
        // Sanity check the current mag calibration params and restore from backup registers
        // if they are bad.  If the backup registers are also 
        if (FALSE == CentroidIsValid(cali->magParams3D.b.m[AXIS_X],
            cali->magParams3D.b.m[AXIS_Y],
            cali->magParams3D.b.m[AXIS_Z]))                                 
        {
            CalibrationCopyCorrectionMatrix(&(cali->Platform->PlatformCalibration.protractor.correctionMatrix),
                &(cali->magParams3D.a),
                &(cali->magParams3D.b));
        }
    }

    MATRIX_STRUCT_T mResult = {3, 1, {.0f, .0f, .0f}};                               
    MATRIX_STRUCT_T mMag = {3, 1, {(float)(pSensorData->data.accel.xyz.x), (float)(pSensorData->data.accel.xyz.y), (float)(pSensorData->data.accel.xyz.z)}};

    // Shift the point using the hard iron offsets
    mMag.m[AXIS_X] += cali->magParams3D.b.m[AXIS_X];
    mMag.m[AXIS_Y] += cali->magParams3D.b.m[AXIS_Y];
    mMag.m[AXIS_Z] += cali->magParams3D.b.m[AXIS_Z]; 

    // Apply soft iron correction using a 3D rotation and scaling matrix
    //TODO: soft iron correction can compensate device mount error and soft iron interference
    //the matrix will be calculated by calibration tool and set to system non-violate memory
    MatrixMultiply(&(cali->magParams3D.a), &mMag, &mResult);

    pSensorData->data.accel.xyz.x = (int32_t)(mResult.m[AXIS_X]);
    pSensorData->data.accel.xyz.y = (int32_t)(mResult.m[AXIS_Y]);
    pSensorData->data.accel.xyz.z = (int32_t)(mResult.m[AXIS_Z]);    

    // Apply a low pass filter to avoid wild swings back and forth
    if (cali->Platform->CalibrationFeatures.enableAccelerometerLowPassFilter)
    {
        ROTATION_VECTOR_T calibratedMag[NUM_AXES] = {mResult.m[AXIS_X],mResult.m[AXIS_Y],mResult.m[AXIS_Z]};
        ApplyLowPassFilter(calibratedMag, 
            cali->s_prevCalibratedMag, 
            (sizeof(cali->s_prevCalibratedMag) / sizeof(cali->s_prevCalibratedMag[0])), 
            cali->Platform->CalibrationSettings.accLowPassFilterAlpha);

        pSensorData->data.accel.xyz.x = (int32_t)(calibratedMag[AXIS_X]);
        pSensorData->data.accel.xyz.y = (int32_t)(calibratedMag[AXIS_Y]);
        pSensorData->data.accel.xyz.z = (int32_t)(calibratedMag[AXIS_Z]);

        TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "acc.lpf(%f,%f,%f)",calibratedMag[0],calibratedMag[1],calibratedMag[2]);
    }
}
