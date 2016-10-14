////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2013, Intel Corporation.  All Rights Reserved.       //
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
// CompassCalibrated.c : Calibration Logic for the Digital Compass.
// It is structured as a "sensor" driver.
//

/* Includes ------------------------------------------------------------------*/
#include "Error.h"
#include "CompassCalibrated.h"
#include "math.h"
#include "stdio.h"
#include "TMR.h"
#include "stdint.h"
#include "platformCalibration.h"
#include "CalibrationCommon.h"
#include "stdlib.h"
#include "MagDynamicCali.h"
#include "SensorManagerTypes.h"
#include "AccelerometerCalibrated.h"
#include "CompassOrientation.h"
#include "Inclinometer.h"

#include "Trace.h"
#include "AccelerometerCalibrated.h"
#include "GyroCalibrated.h"

#define IVH_SENSOR_MAGNETOMETER_POOL_TAG 'MHVI'
//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);

static ERROR_T CalibrateData(pIvhSensorMagnetometer magn, IvhSensorData* SensorData);
static void SmoothMagSpikes(const pIvhSensorMagnetometer magn, int32_t* const x, int32_t* const y, int32_t* const z);
static void CalculateMagCovariance(const pIvhSensorMagnetometer magn, const IvhSensorData* const magData, float covarianceMatrix[COVARIANCE_MATRIX_SIZE]);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

pIvhSensor CreateSensorMagnetometer(const pIvhPlatform platform, pIvhSensor accl, pIvhSensor gyro)
{
    pIvhSensorMagnetometer sensor = (pIvhSensorMagnetometer)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorMagnetometer), IVH_SENSOR_MAGNETOMETER_POOL_TAG);

    if (sensor) 
    {
        // ZERO it!
        SAFE_FILL_MEM (sensor, sizeof (IvhSensorMagnetometer), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for magn");
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
    sensor->sensor.type = IVH_SENSOR_TYPE_MAGNETOMETER3D;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;
    accl->Attach(accl, &(sensor->sensor));
    if(gyro != NULL)
    {
        gyro->Attach(gyro, &(sensor->sensor));
    }

    sensor->CALIBRATION_POINT_MAX_INSTANTANEOUS_DIFF = 10;

    sensor->currentIndex = 0;

    sensor->magFifoIndex = 0;
    sensor->rolloverCount = 0;

    sensor->Platform = platform;
    InitializeMagDynamic(&sensor->Calibration, platform);

    return (pIvhSensor)(sensor);
}

void DestroySensorMagnetometer(pIvhSensor me)
{
    pIvhSensorMagnetometer sensor = (pIvhSensorMagnetometer)(me);

    if(sensor != NULL)
    {
        UnInitializeMagDynamic(&sensor->Calibration);
        SAFE_FREE_POOL(sensor, IVH_SENSOR_MAGNETOMETER_POOL_TAG);
        sensor = NULL;
    }
}

//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------

static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
    ERROR_T retVal = ERROR_OK;
    pIvhSensorMagnetometer sensor = (pIvhSensorMagnetometer)(me);

    switch(type)
    {
    case IVH_SENSOR_TYPE_ACCELEROMETER3D :
        SAFE_MEMCPY(&sensor->accl, data);
        break;
    case IVH_SENSOR_TYPE_GYROSCOPE3D :
        SAFE_MEMCPY(&sensor->gyro, data);
        break;
    case IVH_SENSOR_TYPE_MAGNETOMETER3D :
        SAFE_MEMCPY(&sensor->input, data);
        Calibrate(me);
        break;
    default:
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]invalid sensor type, expect [IVH_SENSOR_TYPE_ACCELEROMETER3D|IVH_SENSOR_TYPE_GYROSCOPE3D|IVH_SENSOR_TYPE_MAGNETOMETER3D]");
    }

    return retVal;
}

static ERROR_T Calibrate(IvhSensor* me)
{
    ERROR_T retVal = ERROR_OK;
    IvhSensorData temp;
    pIvhSensorMagnetometer sensor = (pIvhSensorMagnetometer)(me);

    SAFE_MEMCPY(&temp, &sensor->input);
    CalibrateData(sensor, &temp);
    SAFE_MEMCPY(&sensor->output, &temp);

    me->Notify(me);
    return retVal;
}

static ERROR_T CalibrateData(pIvhSensorMagnetometer magn, IvhSensorData* SensorData)
{
    ERROR_T retVal = ERROR_OK;

    // Fill the donated struct with raw data from the magnetometer    
    SensorData->data.mag.xyzRaw.x = magn->input.data.mag.xyzRaw.x;
    SensorData->data.mag.xyzRaw.y = magn->input.data.mag.xyzRaw.y;
    SensorData->data.mag.xyzRaw.z = magn->input.data.mag.xyzRaw.z;
    SensorData->timeStampInMs = magn->input.timeStampInMs;

    // Swap axes according to orientation of the chip on the board
    // the magField X,Y,Z are replaced with swapped values in this function
    CalibrationSwapAxes(&(SensorData->data.mag.xyzRaw),
        &magn->Platform->PlatformCalibration.mag.orientation);

#if defined(ENABLE_APPLY_STATIC_ROTATION)

    ApplyStaticRotation(&pSensorData->data.mag.xyzRaw.x,
        &pSensorData->data.mag.xyzRaw.y,
        &pSensorData->data.mag.xyzRaw.z);

#endif

    if(magn->Platform->CalibrationSettings.calibration == 1)
    { //per system calibration, direct use offset
        SensorData->data.mag.xyzRaw.x -= (magn->Platform->PlatformCalibration.mag.offset.offsetX);
        SensorData->data.mag.xyzRaw.y -= (magn->Platform->PlatformCalibration.mag.offset.offsetY);
        SensorData->data.mag.xyzRaw.z -= (magn->Platform->PlatformCalibration.mag.offset.offsetZ);
    }

    //SensorData->data.mag.xyzRaw.x += (int32_t)(magn->Calibration.magParams3D.b.m[0]);
    //SensorData->data.mag.xyzRaw.y += (int32_t)(magn->Calibration.magParams3D.b.m[1]);
    //SensorData->data.mag.xyzRaw.z += (int32_t)(magn->Calibration.magParams3D.b.m[2]);

#if defined(ENABLE_SMOOTH_MAG_SPIKES)
    // Filter out instantaneous spikes in mag value
    SmoothMagSpikes(magn, &(SensorData->data.mag.xyzRaw.x),
        &(SensorData->data.mag.xyzRaw.y),
        &(SensorData->data.mag.xyzRaw.z)); 
    SensorData->data.mag.xyzRotated.x = SensorData->data.mag.xyzCalibrated.x = (ROTATION_DATA_T)SensorData->data.mag.xyzRaw.x;
    SensorData->data.mag.xyzRotated.y = SensorData->data.mag.xyzCalibrated.y = (ROTATION_DATA_T)SensorData->data.mag.xyzRaw.y;
    SensorData->data.mag.xyzRotated.z = SensorData->data.mag.xyzCalibrated.z = (ROTATION_DATA_T)SensorData->data.mag.xyzRaw.z;
#endif

    // Pass in a IvhSensorData with raw mag data.  MagDynamiCali fills in the calibrated mag data
    // TODO(bwoodruf): The mag offset refinement and anomaly detection happens in DeviceOrientation.
    // It would be better here, but those algos require gyro, which creates a circular reference
    // with GyroCalibrated.
    if(magn->Platform->CalibrationSettings.calibration > 1)
    { //per system calibration, direct use offset
        MagDynamicCali(&magn->Calibration, SensorData, &magn->accl);
    }

    CalculateMagCovariance(magn, SensorData, SensorData->data.mag.covariance);

    return retVal;
}

#if !defined(ENABLE_SMOOTH_MAG_SPIKES)
static void SmoothMagSpikes(const pIvhSensorMagnetometer magn, int32_t* const x, int32_t* const y, int32_t* const z)
{

    // Use a three point filter to eliminate spikes
    // pointBuffer is a three element rolling buffer of points
    magn->currentIndex = (magn->currentIndex + 1) % 3;

    // Store the current point in the pointBuffer at the currentIndex
    magn->pointBuffer[magn->currentIndex].x = *x;
    magn->pointBuffer[magn->currentIndex].y = *y;
    magn->pointBuffer[magn->currentIndex].z = *z;

    // Do a comparison among the last three points.  Compare the previously stored point against the
    // point that came before and after it to identify a spike.  Note that adding 2 and mod 3
    // is the same as subtracting one from the rolling index.
    uint16_t compareIndex = (magn->currentIndex + 2) % 3;  // current - 1
    uint16_t prevIndex = (magn->currentIndex + 1) % 3;     // current - 2
    uint16_t nextIndex = magn->currentIndex;               // current

    int32_t averageTwoPointsX = (magn->pointBuffer[prevIndex].x + magn->pointBuffer[nextIndex].x) / 2;
    int32_t averageTwoPointsY = (magn->pointBuffer[prevIndex].y + magn->pointBuffer[nextIndex].y) / 2;
    int32_t averageTwoPointsZ = (magn->pointBuffer[prevIndex].z + magn->pointBuffer[nextIndex].z) / 2;
    uint32_t instantaneousDiffX = abs(magn->pointBuffer[compareIndex].x - averageTwoPointsX);
    uint32_t instantaneousDiffY = abs(magn->pointBuffer[compareIndex].y - averageTwoPointsY);
    uint32_t instantaneousDiffZ = abs(magn->pointBuffer[compareIndex].z - averageTwoPointsZ);

    // If the instantaneous diffs are greater than the max allowed,
    // use the average of the two surrounding points instead.
    if (instantaneousDiffX > (uint32_t)(magn->CALIBRATION_POINT_MAX_INSTANTANEOUS_DIFF))
    {
        *x = averageTwoPointsX;
        magn->pointBuffer[compareIndex].x = averageTwoPointsX;
    }

    if (instantaneousDiffY > (uint32_t)(magn->CALIBRATION_POINT_MAX_INSTANTANEOUS_DIFF))
    {
        *y = averageTwoPointsY;
        magn->pointBuffer[compareIndex].y = averageTwoPointsY;
    }

    if (instantaneousDiffZ > (uint32_t)(magn->CALIBRATION_POINT_MAX_INSTANTANEOUS_DIFF))
    {
        *z = averageTwoPointsZ;
        magn->pointBuffer[compareIndex].z = averageTwoPointsZ;
    }
}
#else
static void SmoothMagSpikes(const pIvhSensorMagnetometer magn, int32_t* const x, int32_t* const y, int32_t* const z)
{
    struct point *p=magn->pointBuffer;
    uint8_t i = magn->currentIndex;
    p[i].x=*x,p[i].y=*y,p[i].z=*z;
    *x=p[0].x>p[1].x?(p[0].x<p[2].x?p[0].x:(p[1].x<p[2].x?p[1].x:p[2].x)):(p[0].x>p[2].x?p[0].x:(p[1].x>p[2].x?p[1].x:p[2].x));
    *y=p[0].y>p[1].y?(p[0].y<p[2].y?p[0].y:(p[1].y<p[2].y?p[1].y:p[2].y)):(p[0].y>p[2].y?p[0].y:(p[1].y>p[2].y?p[1].y:p[2].y));
    *z=p[0].z>p[1].z?(p[0].z<p[2].z?p[0].z:(p[1].z<p[2].z?p[1].z:p[2].z)):(p[0].z>p[2].z?p[0].z:(p[1].z>p[2].z?p[1].z:p[2].z));
    ++magn->currentIndex;
    magn->currentIndex%=3;
}
#endif

static void CalculateMagCovariance(const pIvhSensorMagnetometer magn, const IvhSensorData* const magData, float covarianceMatrix[COVARIANCE_MATRIX_SIZE])
{
    // Calibrated mag is between 0 and 1.  Multiply by 1000 to bring mag to within a range of +-1000
    // This allows us to use integer math
    magn->magFifo[magn->magFifoIndex][AXIS_X] = (int32_t)(magData->data.mag.xyzCalibrated.x * 1000);
    magn->magFifo[magn->magFifoIndex][AXIS_Y] = (int32_t)(magData->data.mag.xyzCalibrated.y * 1000);
    magn->magFifo[magn->magFifoIndex][AXIS_Z] = (int32_t)(magData->data.mag.xyzCalibrated.z * 1000);

    ++magn->magFifoIndex;
    if (magn->magFifoIndex == MAG_FIFO_SIZE)
    {
        ++magn->rolloverCount;
        magn->magFifoIndex = 0;
    }

    if (magn->rolloverCount > 0)
    {
        uint8_t i;
        int32_t sums[NUM_AXES] = {0, 0, 0};
        for (i=0; i < MAG_FIFO_SIZE; ++i)
        {
            sums[AXIS_X] += magn->magFifo[i][AXIS_X];
            sums[AXIS_Y] += magn->magFifo[i][AXIS_Y];
            sums[AXIS_Z] += magn->magFifo[i][AXIS_Z];
        }

        int32_t means[NUM_AXES] = {sums[AXIS_X] / MAG_FIFO_SIZE, sums[AXIS_Y] / MAG_FIFO_SIZE, sums[AXIS_Z] / MAG_FIFO_SIZE};

        // Calculate an upper triangular covariance matrix.  COVARIANCE_MATRIX_SIZE should be == 6
        const float coefficient = 1.0f / (MAG_FIFO_SIZE - 1);
        uint8_t covarianceIndex = 0;
        uint8_t j,k;
        for (j=0; j<NUM_AXES; ++j)
        {
            for (k=j; k<NUM_AXES; ++k)
            {
                // Prevent any writing outside the covarianceMatrix array
                ASSERT(covarianceIndex < COVARIANCE_MATRIX_SIZE);
                covarianceMatrix[covarianceIndex] = 0;
                for (i=0; i<MAG_FIFO_SIZE; ++i)
                {
                    covarianceMatrix[covarianceIndex] += coefficient * (magn->magFifo[i][j] - means[j]) * (magn->magFifo[i][k] - means[k]);
                }
                ++covarianceIndex;
            }
        }
    }
}
