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
#include "Error.h"
#include "CompassOrientation.h"
#include "stdlib.h"
#include <math.h>
#include "Common.h"
#include "SensorManagerTypes.h"
#include "TMR.h"
#include "Conversion.h"
#include "Rotation.h"

#include "Trace.h"

#define IVH_SENSOR_COMPASS_POOL_TAG 'CHVI'

//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);

//------------------------------------------------------------------------------
// DATA
//------------------------------------------------------------------------------
static const uint8_t AXIS_CHANGE_THRESHOLD_DEGREES = 20;

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

pIvhSensor CreateSensorCompass(pIvhSensor orientation)
{
    pIvhSensorCompass sensor = (pIvhSensorCompass)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorCompass), IVH_SENSOR_COMPASS_POOL_TAG);

    if (sensor) 
    {
        // ZERO it!
        SAFE_FILL_MEM (sensor, sizeof (IvhSensorCompass), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for compass");
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
    sensor->sensor.type = IVH_SENSOR_TYPE_COMPASS3D;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;

    orientation->Attach(orientation, &(sensor->sensor));

    sensor->lastAxisReferenceChangePitchDegrees = .0f;
    sensor->s_axisChangeAllowed = FALSE;

    sensor->axisReference = AXIS_REFERENCE_POSITIVE_Y;

    return (pIvhSensor)(sensor);
}

void DestroySensorCompass(pIvhSensor me)
{
    pIvhSensorCompass sensor = (pIvhSensorCompass)(me);

    if(sensor != NULL)
    {
        SAFE_FREE_POOL(sensor, IVH_SENSOR_COMPASS_POOL_TAG);
        sensor = NULL;
    }
}

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------
// Allow axis reference change ONLY if we have exceeded 20 degrees since the last axis change.
static BOOLEAN IsAxisChangeAllowed(IN const float changeInDegrees,
                                   IN const float lastAxisReferenceChangeDegrees)
{
    // Find the smallest angle difference between two headings
    float degreesSinceLastAxisChange;
    float angleDiff = fabs(changeInDegrees - lastAxisReferenceChangeDegrees);
    if (angleDiff < 360.0f - angleDiff)
    {
        degreesSinceLastAxisChange = angleDiff;
    } 
    else
    {
        degreesSinceLastAxisChange = 360.0f - angleDiff;
    }

    if (AXIS_CHANGE_THRESHOLD_DEGREES < degreesSinceLastAxisChange)
    {
        return TRUE;
    }

    return FALSE;
}

// Find the heading reference axis for the current orientation
// The reference axis changes depending on portrait or landscape orientation
// and the pitch or roll angle.
//
// In all cases, imagine the user holding the device and 
// looking at the screen of the device in slate mode to find the correct heading 
static void FindAxisReference(IN pIvhSensorCompass compass, IN const float pitchDegrees,
                              INOUT AXIS_REFERENCE_T* const axisReference)
{
    // The axisChangeAllowed flag should only flip once when we 
    // exceed the pitch threshold of 20 degrees.
    // Once it flips to TRUE, we don't need to test it again until the axis definitions change
    if (FALSE == compass->s_axisChangeAllowed)
    {
        compass->s_axisChangeAllowed = IsAxisChangeAllowed(pitchDegrees, compass->lastAxisReferenceChangePitchDegrees);
    }

    if (FALSE != compass->s_axisChangeAllowed)
    {
        BOOLEAN axisReferenceChanged = FALSE;

        // Between +45 and -45 degrees pitch, check the roll value
        // to allow heading to follow a portrait orientation
        if ((45 >= pitchDegrees && 0 <= pitchDegrees) ||
            (360 >= pitchDegrees && 315 <= pitchDegrees))
        {
            if (AXIS_REFERENCE_POSITIVE_Y != *axisReference)
            {
                *axisReference = AXIS_REFERENCE_POSITIVE_Y;
                axisReferenceChanged = TRUE;
            }
        }
        else if (45 < pitchDegrees && 135 >= pitchDegrees)
        {
            // Between +45 and +135 degrees pitch, use the -Z axis
            if (AXIS_REFERENCE_NEGATIVE_Z != *axisReference)
            {
                *axisReference = AXIS_REFERENCE_NEGATIVE_Z;
                axisReferenceChanged = TRUE;
            }
        } 
        else if (135 < pitchDegrees && 225 > pitchDegrees)
        {
            // Between 135 and 225 degrees pitch use the -Y axis as the heading reference
            if (AXIS_REFERENCE_NEGATIVE_Y != *axisReference)
            {
                *axisReference = AXIS_REFERENCE_NEGATIVE_Y;
                axisReferenceChanged = TRUE;
            }
        }
        else
        {
            // Between +225 and +315 degrees pitch use the +Z axis
            if (AXIS_REFERENCE_POSITIVE_Z != *axisReference)
            {
                *axisReference = AXIS_REFERENCE_POSITIVE_Z;
                axisReferenceChanged = TRUE;
            }
        }

        // If the axis reference changed then reset the
        // axisChangeAllowed flag and remember the pitch when
        // we swapped the axis reference
        if (FALSE != axisReferenceChanged)
        {
            compass->lastAxisReferenceChangePitchDegrees = pitchDegrees;
            compass->s_axisChangeAllowed = FALSE;
        }
    }
}

static void CalculateHeading(IN pIvhSensorCompass compass, IN const IvhSensorData* const devOrientData,
                             OUT float* const headingDegrees)
{
    // Get the pitch, Y and Z axis headings from the rotation matrix and normalize to 0-359 degrees
    // Heading is opposite rotation direction from yaw so we subtract from 360
    float pitchDegrees = ConversionNormalizeDegrees(devOrientData->data.orientation.pitch);
    float headingYAxis = ConversionNormalizeDegrees(360 - devOrientData->data.orientation.yawYAxis);
    float headingZAxis = ConversionNormalizeDegrees(360 - devOrientData->data.orientation.yawZAxis);    

    // Find the correct reference axis based on pitch
    FindAxisReference(compass, pitchDegrees, &compass->axisReference);

    // Now set heading according to the axis reference
    switch (compass->axisReference)
    {
    case(AXIS_REFERENCE_NEGATIVE_Y):
        *headingDegrees = ConversionNormalizeDegrees(headingYAxis + 180);
        break;
    case(AXIS_REFERENCE_NEGATIVE_Z):
        *headingDegrees = ConversionNormalizeDegrees(headingZAxis);
        break;
    case(AXIS_REFERENCE_POSITIVE_Z):
        *headingDegrees = ConversionNormalizeDegrees(headingZAxis + 180);
        break;
    case(AXIS_REFERENCE_POSITIVE_Y):
    default:
        *headingDegrees = ConversionNormalizeDegrees(headingYAxis);
        break;   
    }
}

/*******************************************************************************
* Function Name  : CalculateNorth
* Description    : calculate north heading
* Return         : north heading
*******************************************************************************/
static float CalculateNorth(const IN pIvhSensorCompass compass)
{
    float north;
    float dcm[] = {\
        compass->input.data.orientation.rotationMatrix.m11, compass->input.data.orientation.rotationMatrix.m12, compass->input.data.orientation.rotationMatrix.m13, \
        compass->input.data.orientation.rotationMatrix.m21, compass->input.data.orientation.rotationMatrix.m22, compass->input.data.orientation.rotationMatrix.m23, \
        compass->input.data.orientation.rotationMatrix.m31, compass->input.data.orientation.rotationMatrix.m32, compass->input.data.orientation.rotationMatrix.m33, \
    };

    if(dcm[8] < .70710678f && dcm[8] > -.70710678f )
    {
        //reference choose -z-axis
        north = RAD_TO_DEG * atan2f(-dcm[2],-dcm[5]);
    }
    else
    {
        //choose y-axis
        north = RAD_TO_DEG * atan2f(dcm[1],dcm[4]);
    }
    return north<0?north+360.0f:north;
}

static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
    ERROR_T retVal = ERROR_OK;

    pIvhSensorCompass sensor = (pIvhSensorCompass)(me);

    if(type != IVH_SENSOR_TYPE_ORIENTATION_9AXIS && type != IVH_SENSOR_TYPE_ORIENTATION_6AXIS)
    {
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]invalid sensor type, expect [IVH_SENSOR_TYPE_ORIENTATION_6AXIS] or [IVH_SENSOR_TYPE_ORIENTATION_9AXIS]");
    }
    sensor->input.data.orientation.pitch = data->data.orientation.pitch;
    sensor->input.data.orientation.yawYAxis = data->data.orientation.yawYAxis;
    sensor->input.data.orientation.yawZAxis = data->data.orientation.yawZAxis;
    sensor->input.data.orientation.estimatedHeadingError = data->data.orientation.estimatedHeadingError;
    sensor->input.data.orientation.magRaw.x = data->data.orientation.magRaw.x;
    sensor->input.data.orientation.magRaw.y = data->data.orientation.magRaw.y;
    sensor->input.data.orientation.magRaw.z = data->data.orientation.magRaw.z;

    sensor->input.data.orientation.rotationMatrix.m11 = data->data.orientation.rotationMatrix.m11;
    sensor->input.data.orientation.rotationMatrix.m12 = data->data.orientation.rotationMatrix.m12;
    sensor->input.data.orientation.rotationMatrix.m13 = data->data.orientation.rotationMatrix.m13;
    sensor->input.data.orientation.rotationMatrix.m21 = data->data.orientation.rotationMatrix.m21;
    sensor->input.data.orientation.rotationMatrix.m22 = data->data.orientation.rotationMatrix.m22;
    sensor->input.data.orientation.rotationMatrix.m23 = data->data.orientation.rotationMatrix.m23;
    sensor->input.data.orientation.rotationMatrix.m31 = data->data.orientation.rotationMatrix.m31;
    sensor->input.data.orientation.rotationMatrix.m32 = data->data.orientation.rotationMatrix.m32;
    sensor->input.data.orientation.rotationMatrix.m33 = data->data.orientation.rotationMatrix.m33;

    sensor->input.timeStampInMs = data->timeStampInMs;

    Calibrate(me);
    return retVal;
}

static ERROR_T Calibrate(IvhSensor* me)
{
    ERROR_T retVal = ERROR_OK;

    pIvhSensorCompass sensor = (pIvhSensorCompass)(me);

    IvhSensorData devOrientData;
    SAFE_MEMCPY(&devOrientData, &sensor->input);

    float headingDegrees;

    CalculateHeading(sensor, &devOrientData, &headingDegrees);
    sensor->output.data.mag.compensatedHeadingMagneticNorth = 
        CalculateNorth(sensor);
    sensor->output.data.mag.estimatedHeadingError = 
        devOrientData.data.orientation.estimatedHeadingError;
    sensor->output.data.mag.xyzRaw.x = 
        devOrientData.data.orientation.magRaw.x;
    sensor->output.data.mag.xyzRaw.y = 
        devOrientData.data.orientation.magRaw.y;
    sensor->output.data.mag.xyzRaw.z = 
        devOrientData.data.orientation.magRaw.z;

    me->Notify(me);
    return retVal;
}

