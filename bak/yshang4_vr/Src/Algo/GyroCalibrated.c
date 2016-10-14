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

/* Includes ------------------------------------------------------------------*/
#include <math.h>
#include "Error.h"
#include "GyroCalibrated.h"
#include "platformCalibration.h"
#include "CalibrationCommon.h"
#include "stdlib.h"
#include "BackupRegisters.h"

#include "CompassCalibrated.h"
#include "SensorManagerTypes.h"

#include "DeviceOrientation.h"

#include "Trace.h"

//------------------------------------------------------------------------------
// DATA
//------------------------------------------------------------------------------
// If we're not moving, then add up samples and average them to find
// new gyro offsets.
// Milli-dps values below this threshold will be considered as motionless
#define MAG_COVARIANCE_THRESHOLD_MINIMUM        10
#define MAG_COVARIANCE_THRESHOLD_FIRST_TIME     1000
#define MAG_COVARIANCE_THRESHOLD_REDUCTION_RATE 2

#define GYRO_MOTION_THRESHOLD_MINIMUM           1000
#define GYRO_MOTION_THRESHOLD_FIRST_TIME        10000
#define GYRO_MOTION_THRESHOLD_REDUCTION_RATE    2

//TODO: we need enable gyroscope dynamic calibration duo to many application use the integration of gyro...
//#define GYRO_DYNAMIC_CALIBRATION

#define IVH_SENSOR_SENSORSCOPE_POOL_TAG 'GHVI'
//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);

static void CalibrateData(const pIvhSensorGyroscope gyro, pIvhSensorData const pSensorData);
static void FilterSingleSampleSpikes(const pIvhSensorGyroscope gyro, int32_t* const x, int32_t* const y,int32_t* const z);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

pIvhSensor CreateSensorGyroscope(const pIvhPlatform platform)
{
    pIvhSensorGyroscope sensor = (pIvhSensorGyroscope)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorGyroscope), IVH_SENSOR_SENSORSCOPE_POOL_TAG);

    if (sensor) 
    {
        // ZERO it!
        SAFE_FILL_MEM (sensor, sizeof (IvhSensorGyroscope), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for gyro");
    }
    else 
    {
        //  No memory
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "not enough memory");
        return NULL;
    }
    sensor->sensor.Update = DataUpdate;
    sensor->sensor.Notify = SensorNotify;
    sensor->sensor.Attach = SensorAttach;
    sensor->sensor.QueryData = SensorQueryData;
    sensor->sensor.type = IVH_SENSOR_TYPE_GYROSCOPE3D;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;

    sensor->CorrectionMatrix.cols = 3;
    sensor->CorrectionMatrix.rows = 3;
    SAFE_MEMSET(sensor->CorrectionMatrix.m, 0);

    sensor->calibrationOffsets.cols = 1;
    sensor->calibrationOffsets.rows = 3;
    SAFE_MEMSET(sensor->calibrationOffsets.m, 0);

    sensor->s_sampleSumX = 0;
    sensor->s_sampleSumY = 0;
    sensor->s_sampleSumZ = 0;
    sensor->s_continuousSamplesAtRestCounter = 0;

    SAFE_MEMSET(sensor->s_prevGyro, 0);

    sensor->s_offsetRefinementCount = 0;
    sensor->s_gyroMotionThreshold = GYRO_MOTION_THRESHOLD_FIRST_TIME;
    sensor->s_magCovarianceThreshold = MAG_COVARIANCE_THRESHOLD_FIRST_TIME;

    sensor->lastX = 0;
    sensor->lastY = 0;
    sensor->lastZ = 0;

    sensor->PlatformCalibration = &platform->PlatformCalibration;
    sensor->PlatformSettings = &platform->CalibrationSettings;

    CalibrationCopyCorrectionMatrix(&(platform->PlatformCalibration.protractor.correctionMatrix),
        &sensor->CorrectionMatrix,
        &sensor->calibrationOffsets);
    sensor->calibrationOffsets.m[0] = platform->PlatformCalibration.gyro.offset.offsetX;
    sensor->calibrationOffsets.m[1] = platform->PlatformCalibration.gyro.offset.offsetY;
    sensor->calibrationOffsets.m[2] = platform->PlatformCalibration.gyro.offset.offsetZ;

    return (pIvhSensor)(sensor);
}

void DestroySensorGyroscope(pIvhSensor me)
{
    pIvhSensorGyroscope sensor = (pIvhSensorGyroscope)(me);

    if(sensor != NULL)
    {
        SAFE_FREE_POOL(sensor, IVH_SENSOR_SENSORSCOPE_POOL_TAG);
        sensor = NULL;
    }
}

//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------
#ifdef GYRO_DYNAMIC_CALIBRATION
static void AdjustZeroRateOffset(IN const PGYROMETER_DATA_T gyro,
                                 IN const IvhSensorData* const gyroData,
                                 IN const IvhSensorData* const magData)

{                                    
    const float alphaGyro = 0.05f;
    float thisGyro[NUM_AXES] = {(float)(gyroData->data.gyro.xyz.x),
        (float)(gyroData->data.gyro.xyz.y),
        (float)(gyroData->data.gyro.xyz.z)};
    float avgGyro[NUM_AXES] = {thisGyro[AXIS_X], 
        thisGyro[AXIS_Y], 
        thisGyro[AXIS_Z]};
    ApplyLowPassFilter(avgGyro, gyro->s_prevGyro, 3, alphaGyro);


    // If this is the first time for offset calibration the offsets could be way off, causing the
    // motionless average to fall outside the normally strict offset refinement target.  
    // For the first few calibrations cycles,
    // allow a larger norm and mag covariance threshold so that the offsets will be near the right value.
    // Then reduce the threshold with each calibration cycle.
    if ((gyro->s_offsetRefinementCount > 0) &&
        (gyro->s_gyroMotionThreshold > GYRO_MOTION_THRESHOLD_MINIMUM) &&
        (gyro->s_magCovarianceThreshold > MAG_COVARIANCE_THRESHOLD_MINIMUM))
    {    
        gyro->s_gyroMotionThreshold =    (uint16_t)(MAX(GYRO_MOTION_THRESHOLD_MINIMUM,
            gyro->s_gyroMotionThreshold / (GYRO_MOTION_THRESHOLD_REDUCTION_RATE * gyro->s_offsetRefinementCount)));
        gyro->s_magCovarianceThreshold = (uint16_t)(MAX(MAG_COVARIANCE_THRESHOLD_MINIMUM,
            gyro->s_magCovarianceThreshold / (MAG_COVARIANCE_THRESHOLD_REDUCTION_RATE * gyro->s_offsetRefinementCount)));
    }

    if (FALSE == CalibrationDeviceIsMotionless(thisGyro, 
        avgGyro, 
        magData->data.mag.covariance, 
        gyro->s_gyroMotionThreshold,
        gyro->s_magCovarianceThreshold))
    {
        gyro->s_continuousSamplesAtRestCounter = 0;
        gyro->s_sampleSumX = gyro->s_sampleSumY = gyro->s_sampleSumZ = 0;
    }
    else
    {
        ++gyro->s_continuousSamplesAtRestCounter;
        gyro->s_sampleSumX += gyroData->data.gyro.xyz.x;
        gyro->s_sampleSumY += gyroData->data.gyro.xyz.y;
        gyro->s_sampleSumZ += gyroData->data.gyro.xyz.z;

        // If we reach the minimum number of requred at-rest samples, recalculate offsets
        const uint16_t NUM_ZERO_RATE_SAMPLES = 500; 
        if (gyro->s_continuousSamplesAtRestCounter == NUM_ZERO_RATE_SAMPLES)
        {    
            gyro->calibrationOffsets.m[AXIS_X] -= (gyro->s_sampleSumX / gyro->s_continuousSamplesAtRestCounter);
            gyro->calibrationOffsets.m[AXIS_Y] -= (gyro->s_sampleSumY / gyro->s_continuousSamplesAtRestCounter);
            gyro->calibrationOffsets.m[AXIS_Z] -= (gyro->s_sampleSumZ / gyro->s_continuousSamplesAtRestCounter);

            gyro->s_continuousSamplesAtRestCounter = 0;
            gyro->s_sampleSumX = gyro->s_sampleSumY = gyro->s_sampleSumZ = 0;
            ++gyro->s_offsetRefinementCount;
        }
    }
}

// This filter throws away the first sample after reading zero on all three axes
// It will reject momentary blips that can only be noise
static void FilterSingleSampleSpikes(IN const PGYROMETER_DATA_T gyro,
                                     INOUT int32_t* const x, 
                                     INOUT int32_t* const y,
                                     INOUT int32_t* const z)
{
    BOOLEAN sittingStill;

    if( 0 == gyro->lastX && 0 == gyro->lastY  && 0 == gyro->lastZ )
    {
        sittingStill = TRUE;
    }
    else
    {
        sittingStill = FALSE;
    }

    gyro->lastX = *x;
    gyro->lastY = *y;
    gyro->lastZ = *z;

    // The very first sample from sitting completely still we drop
    // This eliminates the noise at any level that happens for one sample 17 or otherwise
    // As soon as we are moving and hit 2 samples this will not be called and we are back to normal
    if( (TRUE == sittingStill) &&
        ( !(0 == *x) ||
        !(0 == *y) ||
        !(0 == *z) ))//  we were not moving so this could be noise
    {                                                               //  so drop the first sample
        *x = 0;
        *y = 0;
        *z = 0; 
    }
}

static BOOLEAN OffsetsAreValid(IN const float offsetX,
                               IN const float offsetY,
                               IN const float offsetZ)
{
    // "Typical" case zero-rate level at 2000 dps sensitivity from the L3GD20 datasheet is +-75dps
    const float MAX_OFFSET_ABS = 75000;
    if ((fabs(offsetX) > MAX_OFFSET_ABS) ||
        (fabs(offsetY) > MAX_OFFSET_ABS) ||
        (fabs(offsetZ) > MAX_OFFSET_ABS) ||
        (FALSE == isfinite(offsetX) ||
        FALSE == isfinite(offsetY) ||
        FALSE == isfinite(offsetZ)))
    {
        return FALSE;
    }   

    return TRUE;
}
#endif

static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
    ERROR_T retVal = ERROR_OK;

    if(type != IVH_SENSOR_TYPE_GYROSCOPE3D)
    {
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]invalid sensor type, expect [IVH_SENSOR_TYPE_GYROSCOPE3D]");
    }
    SAFE_MEMCPY(me->d[0], data);

    Calibrate(me);
    return retVal;
}

static ERROR_T Calibrate(IvhSensor* me)
{
    ERROR_T retVal = ERROR_OK;
    IvhSensorData temp;
    pIvhSensorGyroscope sensor = (pIvhSensorGyroscope)(me);

    SAFE_MEMCPY(&temp, &sensor->input);
    CalibrateData(sensor, &temp);
    SAFE_MEMCPY(&sensor->output, &temp);

    me->Notify(me);
    return retVal;
}

static void CalibrateData(const pIvhSensorGyroscope gyro, pIvhSensorData const pSensorData)
{
    CalibrationSwapAxes(&pSensorData->data.gyro.xyz,
        &gyro->PlatformCalibration->gyro.orientation);

    CalibrationApplyCorrectionMatrix(&pSensorData->data.gyro.xyz.x,
        &pSensorData->data.gyro.xyz.y,
        &pSensorData->data.gyro.xyz.z,
        &gyro->CorrectionMatrix,
        &gyro->calibrationOffsets);

    Vector3I32MultiplyByScalar(&pSensorData->data.gyro.xyz,
        (int32_t)(gyro->PlatformSettings->gyroBoost));

    FilterSingleSampleSpikes(gyro, &pSensorData->data.gyro.xyz.x,
        &pSensorData->data.gyro.xyz.y,
        &pSensorData->data.gyro.xyz.z);
}

// This filter throws away the first sample after reading zero on all three axes
// It will reject momentary blips that can only be noise
static void FilterSingleSampleSpikes(const pIvhSensorGyroscope gyro, int32_t* const x, int32_t* const y,int32_t* const z)
{
    BOOLEAN sittingStill;

    if( 0 == gyro->lastX && 0 == gyro->lastY  && 0 == gyro->lastZ )
    {
        sittingStill = TRUE;
    }
    else
    {
        sittingStill = FALSE;
    }

    gyro->lastX = *x;
    gyro->lastY = *y;
    gyro->lastZ = *z;

    // The very first sample from sitting completely still we drop
    // This eliminates the noise at any level that happens for one sample 17 or otherwise
    // As soon as we are moving and hit 2 samples this will not be called and we are back to normal
    if( (TRUE == sittingStill) &&
        ( !(0 == *x) ||
        !(0 == *y) ||
        !(0 == *z) ))//  we were not moving so this could be noise
    {                                                               //  so drop the first sample
        *x = 0;
        *y = 0;
        *z = 0; 
    }
}
