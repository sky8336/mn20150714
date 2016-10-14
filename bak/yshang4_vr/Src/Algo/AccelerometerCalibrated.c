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
// AccelerometerCalibrated.c : Calibration Logic for the Digital Accelerometer.
//
////////////////////////////////////////////////////////////////////////////////

/* Includes ------------------------------------------------------------------*/
#include "Error.h"
#include "AccelerometerCalibrated.h"
#include "platformCalibration.h"
#include "CalibrationCommon.h"
#include "stdlib.h"
#include "Matrix.h"
#include <math.h>
#include "Common.h"
#include "SensorManagerTypes.h"

#include "DeviceOrientation.h"

#include "Trace.h"

#define IVH_SENSOR_ACCELEROMETER_POOL_TAG 'AHVI'
//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);
static void CalibrateData(IvhSensorData* const pSensorData, const CALIBRATION_ORIENTATION_STRUCT_T* const pOrientStruct, 
                          const MATRIX_STRUCT_T* const pCorrectionMatrix, const MATRIX_STRUCT_T* const pCorrectionVector, int calibration, PACCL_DYNAMICCALI_T pCalibration);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------
pIvhSensor CreateSensorAccelerometer(const pIvhPlatform platform)
{
    pIvhSensorAccelerometer sensor = (pIvhSensorAccelerometer)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorAccelerometer), IVH_SENSOR_ACCELEROMETER_POOL_TAG);

    if (sensor) 
    {
        // ZERO it!
        SAFE_FILL_MEM (sensor, sizeof (IvhSensorAccelerometer), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for accel");
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
    sensor->sensor.type = IVH_SENSOR_TYPE_ACCELEROMETER3D;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;

    sensor->CorrectionMatrix.cols = 3;
    sensor->CorrectionMatrix.rows = 3;
    SAFE_MEMSET(sensor->CorrectionMatrix.m, 0);

    sensor->CorrectionVector.cols = 1;
    sensor->CorrectionVector.rows = 3;
    SAFE_MEMSET(sensor->CorrectionVector.m, 0);

    sensor->PlatformCalibration = &platform->PlatformCalibration;
    CalibrationCopyCorrectionMatrix(&(platform->PlatformCalibration.protractor.correctionMatrix),
        &sensor->CorrectionMatrix,
        &sensor->CorrectionVector);

    sensor->Platform = platform;
    InitializeAccDynamic(&sensor->Calibration, platform);

    return (pIvhSensor)(sensor);
}

void DestroySensorAccelerometer(pIvhSensor me)
{
    pIvhSensorAccelerometer sensor = (pIvhSensorAccelerometer)(me);

    if(sensor != NULL)
    {
        UnInitializeAccDynamic(&sensor->Calibration);
        SAFE_FREE_POOL(sensor, IVH_SENSOR_ACCELEROMETER_POOL_TAG);
        sensor = NULL;
    }
}

//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
    ERROR_T retVal = ERROR_OK;

    if(type != IVH_SENSOR_TYPE_ACCELEROMETER3D)
    {
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]invalid sensor type, expect [IVH_SENSOR_TYPE_ACCELEROMETER3D]");
    }
    SAFE_MEMCPY(me->d[0], data);

    Calibrate(me);
    return retVal;
}

static ERROR_T Calibrate(IvhSensor* me)
{
    ERROR_T retVal = ERROR_OK;
    IvhSensorData temp;
    pIvhSensorAccelerometer sensor = (pIvhSensorAccelerometer)(me);

    SAFE_MEMCPY(&temp, &sensor->input);
    CalibrateData(&temp, &sensor->PlatformCalibration->accel.orientation, &sensor->CorrectionMatrix, &sensor->CorrectionVector, sensor->Platform->CalibrationSettings.calibration, &sensor->Calibration);
    SAFE_MEMCPY(&sensor->output, &temp);

    me->Notify(me);
    return retVal;
}

static void CalibrateData(IvhSensorData* const pSensorData, const CALIBRATION_ORIENTATION_STRUCT_T* const pOrientStruct, 
                          const MATRIX_STRUCT_T* const pCorrectionMatrix, const MATRIX_STRUCT_T* const pCorrectionVector, int calibration, PACCL_DYNAMICCALI_T pCalibration)
{
    CalibrationSwapAxes(&pSensorData->data.accel.xyz, pOrientStruct);

    //TODO: static rotation should be added to conpensate system error
#if defined(ENABLE_APPLY_STATIC_ROTATION)

    ApplyStaticRotation(&pSensorData->data.accel.xyz.x,
        &pSensorData->data.accel.xyz.y,
        &pSensorData->data.accel.xyz.z);

#endif

    if(calibration == 1)
    { //per system calibration, direct use offset
        //TODO: correction matrix can conpensate device mount error and device static offset
        //the matrix and vector will be calculated by calibration tool and set to system non-violate memory
        CalibrationApplyCorrectionMatrix(&pSensorData->data.accel.xyz.x,
            &pSensorData->data.accel.xyz.y,
            &pSensorData->data.accel.xyz.z,
            pCorrectionMatrix,
            pCorrectionVector);
    }

    // Pass in a IvhSensorData with raw mag data.  MagDynamiCali fills in the calibrated mag data
    // TODO(bwoodruf): The mag offset refinement and anomaly detection happens in DeviceOrientation.
    // It would be better here, but those algos require gyro, which creates a circular reference
    // with GyroCalibrated.
    if(calibration > 1)
    { //per system calibration, direct use offset
        RefineAccOffsets(pCalibration, pSensorData);
        AccDynamicCali(pCalibration, pSensorData);
    }

    return;
}
