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
#include "AlsCalibrated.h"
#include "platformCalibration.h"
#include "SensorManagerTypes.h"

#include "DeviceOrientation.h"

#include "Trace.h"

#define IVH_SENSOR_AMBIENTLIGHT_POOL_TAG 'LHVI'
//------------------------------------------------------------------------------
// DATA
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------
pIvhSensor CreateSensorAmbientlight(const pIvhPlatform platform)
{
    pIvhSensorAls sensor = (pIvhSensorAls)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorAls), IVH_SENSOR_AMBIENTLIGHT_POOL_TAG);

    if (sensor) 
    {
        // ZERO it!
        SAFE_FILL_MEM (sensor, sizeof (IvhSensorAls), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for als");
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
    sensor->sensor.type = IVH_SENSOR_TYPE_AMBIENTLIGHT;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;

    sensor->multiplier = platform->PlatformCalibration.als.multiplier;
    return (pIvhSensor)(sensor);
}

void DestroySensorAmbientlight(pIvhSensor me)
{
    pIvhSensorAls sensor = (pIvhSensorAls)(me);

    if(sensor != NULL)
    {
        SAFE_FREE_POOL(sensor, IVH_SENSOR_AMBIENTLIGHT_POOL_TAG);
        sensor = NULL;
    }
}


//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
    ERROR_T retVal = ERROR_OK;

    pIvhSensorAls sensor = (pIvhSensorAls)(me);

    if(type != IVH_SENSOR_TYPE_AMBIENTLIGHT)
    {
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]invalid sensor type, expect [IVH_SENSOR_TYPE_AMBIENTLIGHT]");
    }
    sensor->input.data.alsMilliLux = data->data.alsMilliLux;
    sensor->input.timeStampInMs = data->timeStampInMs;

    Calibrate(me);
    return retVal;
}

static ERROR_T Calibrate(IvhSensor* me)
{
    ERROR_T retVal = ERROR_OK;

    pIvhSensorAls sensor = (pIvhSensorAls)(me);

    //TODO: for different ALS, we need to concern the non-linear...
    //maybe sensor->multiplier can't suit for all types of ALS
    //we need consider non-linear compensation
    sensor->output.data.alsMilliLux = (sensor->input.data.alsMilliLux * sensor->multiplier+50)/100;
    sensor->output.data.alsMilliLux= (sensor->output.data.alsMilliLux +5)/10;
    sensor->output.timeStampInMs = sensor->input.timeStampInMs;
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]als(%d->%d)",sensor->input.data.alsMilliLux, sensor->output.data.alsMilliLux);

    me->Notify(me);
    return retVal;
}

