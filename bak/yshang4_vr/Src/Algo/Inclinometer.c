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
#include "Inclinometer.h"
#include "platformCalibration.h"
#include "stdio.h"
#include "TMR.h"
#include "Common.h"

#include "Trace.h"

#define IVH_SENSOR_INCLINOMETER_POOL_TAG 'IHVI'

//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

pIvhSensor CreateSensorInclinometer(pIvhSensor orientation)
{
    pIvhSensorInclinometer sensor = (pIvhSensorInclinometer)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorInclinometer), IVH_SENSOR_INCLINOMETER_POOL_TAG);

    if (sensor) 
    {
        // ZERO it!
        SAFE_FILL_MEM (sensor, sizeof (IvhSensorInclinometer), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for inclinometer");
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
    sensor->sensor.type = IVH_SENSOR_TYPE_INCLINOMETER;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;
    orientation->Attach(orientation, &(sensor->sensor));

    return (pIvhSensor)(sensor);
}

void DestroySensorInclinometer(pIvhSensor me)
{
    pIvhSensorInclinometer sensor = (pIvhSensorInclinometer)(me);

    if(sensor != NULL)
    {
        SAFE_FREE_POOL(sensor, IVH_SENSOR_INCLINOMETER_POOL_TAG);
        sensor = NULL;
    }
}

//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------

static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
    ERROR_T retVal = ERROR_OK;

    pIvhSensorInclinometer sensor = (pIvhSensorInclinometer)(me);

    if(type != IVH_SENSOR_TYPE_ORIENTATION_9AXIS && type != IVH_SENSOR_TYPE_ORIENTATION_6AXIS)
    {
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]invalid sensor type, expect [IVH_SENSOR_TYPE_ORIENTATION_6AXIS] or [IVH_SENSOR_TYPE_ORIENTATION_9AXIS]");
    }

    sensor->input.data.orientation.pitch = data->data.orientation.pitch;
    sensor->input.data.orientation.yawYAxis = data->data.orientation.yawYAxis;
    sensor->input.data.orientation.yawZAxis = data->data.orientation.yawZAxis;
    sensor->input.data.orientation.roll = data->data.orientation.roll;
    sensor->input.data.orientation.rollZAxis = data->data.orientation.rollZAxis;
    sensor->input.data.orientation.estimatedHeadingError = data->data.orientation.estimatedHeadingError;

    sensor->input.timeStampInMs = data->timeStampInMs;

    Calibrate(me);
    return retVal;
}

/*******************************************************************************
* Function Name  : calibrate
* Description    : Called to retrieve latest data from this sensor
* Return         : ERROR_T
*******************************************************************************/
// Compass conventions are +- 90 degrees for pitch
// +- 180 degrees for roll and yaw, 
// where yaw is measured clockwise from magnetic north
// 
// MSFT convention for inclinometer is documented in the document
// "integrating-motion-and-orientation-sensors.doc" that may be found here
// http://msdn.microsoft.com/en-us/library/windows/hardware/br259127.aspx
// 
//  YAW                0 | 360
/* (top view)       /    Y+     \        */
/*                 /      |      \       */
/*                /       |       \      */
/*               |        ----X+   |     */
/*                \               /      */
/*                 \             /       */
/*                  \           /        */
/*                   \   180   /         */
// 
//  PITCH             ---------
/*  (left side      /    Z+     \        */
/*   view)         /      |      \       */
//                /       |      180
//               0  Y+-----      -- 
//                \             -180
/*                 \             /       */
/*                  \           /        */
/*                   \ ------- /         */
// 
//  ROLL            -90 | -90
/* (bottom edge   /           \          */
/*   view)       /             \         */
//              /       Y       0
//             0        .----X+ -
/*              \               /        */
/*               \             /         */
/*                \           /          */
/*                 \90 | 90--/           */

//TODO: here is a bug(after version 29550) according to the document <Integrating Motion and Orientation Sensors>
//see version 29549 implimentaion

static ERROR_T Calibrate(IvhSensor* me)
{
    ERROR_T retVal = ERROR_OK;

    pIvhSensorInclinometer sensor = (pIvhSensorInclinometer)(me);

    sensor->output.data.inclinometer.tilt.x = sensor->input.data.orientation.pitch;

    // To give a better user experience during gimbal lock,
    // If abs(pitch) is greater than AXIS_REFERENCE_THRESHOLD
    // then use the yaw and roll relative to the Z axis
    const uint16_t AXIS_REFERENCE_THRESHOLD = 30;
    if (fabs(sensor->input.data.orientation.pitch) < (90 - AXIS_REFERENCE_THRESHOLD) ||
        fabs(sensor->input.data.orientation.pitch) > (90 + AXIS_REFERENCE_THRESHOLD))
    {
        sensor->output.data.inclinometer.tilt.z = sensor->input.data.orientation.yawYAxis;
        sensor->output.data.inclinometer.tilt.y = sensor->input.data.orientation.roll;
    }
    else
    {
        sensor->output.data.inclinometer.tilt.z = sensor->input.data.orientation.yawZAxis;
        sensor->output.data.inclinometer.tilt.y = sensor->input.data.orientation.rollZAxis;
    }

    sensor->output.data.inclinometer.estimatedHeadingError = sensor->input.data.orientation.estimatedHeadingError;

    // This is a fusion sensor.  Create our own timestamp.
    sensor->output.timeStampInMs= TmrGetMillisecondCounter();

    me->Notify(me);
    return retVal;
}
