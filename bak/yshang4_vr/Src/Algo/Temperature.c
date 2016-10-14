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
#include "./Temperature.h"
#include "platformCalibration.h"
#include "SensorManagerTypes.h"

#include "Trace.h"

#define IVH_SENSOR_TEMPERATURE_POOL_TAG 'THVI'

//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

pIvhSensor CreateSensorTemperature(const pIvhPlatform platform)
{
    UNREFERENCED_PARAMETER(platform);

    pIvhSensorTemperature sensor = (pIvhSensorTemperature)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorTemperature), IVH_SENSOR_TEMPERATURE_POOL_TAG);

    if (sensor) 
    {
        // ZERO it!
        SAFE_FILL_MEM (sensor, sizeof (IvhSensorTemperature), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for temperature");
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
    sensor->sensor.type = IVH_SENSOR_TYPE_THERMOMETER;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;

    sensor->multiplier = 1000;//platform->PlatformCalibration.als.multiplier;
    return (pIvhSensor)(sensor);
}

void DestroySensorTemperature(pIvhSensor me)
{
    pIvhSensorTemperature sensor = (pIvhSensorTemperature)(me);

    if(sensor != NULL)
    {
        SAFE_FREE_POOL(sensor, IVH_SENSOR_TEMPERATURE_POOL_TAG);
        sensor = NULL;
    }
}

//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------

static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
    ERROR_T retVal = ERROR_OK;

    pIvhSensorTemperature sensor = (pIvhSensorTemperature)(me);

    if(type != IVH_SENSOR_TYPE_THERMOMETER)
    {
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]invalid sensor type, expect [IVH_SENSOR_TYPE_THERMOMETER]");
    }
    sensor->input.data.temperatureMiniCentigrade = data->data.temperatureMiniCentigrade;
    sensor->input.timeStampInMs = data->timeStampInMs;

    Calibrate(me);
    return retVal;
}

static ERROR_T Calibrate(IvhSensor* me)
{
    ERROR_T retVal = ERROR_OK;

    pIvhSensorTemperature sensor = (pIvhSensorTemperature)(me);

    //TODO: for different themometer, we need to concern the non-linear...
    //maybe sensor->multiplier can't suit for all types of themometer
    //we need consider non-linear compensation
    sensor->output.data.temperatureMiniCentigrade = (uint32_t)(sensor->input.data.temperatureMiniCentigrade * sensor->multiplier/1000 - 273);
    sensor->output.timeStampInMs = sensor->input.timeStampInMs;

    me->Notify(me);
    return retVal;
}

