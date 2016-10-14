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
#include "Fusion.h"
#include "../AlgorithmManager.h"
#include "Trace.h"

#define IVH_SENSOR_FUSION_POOL_TAG 'FHVI'

//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

pIvhSensor CreateSensorFusion(const ALGO_NOTIFICATION AlgoNotification, const pIvhSensor sensors[], const int number)
{
	pIvhSensorFusion sensor = (pIvhSensorFusion)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorFusion), IVH_SENSOR_FUSION_POOL_TAG);

	if (sensor) 
	{
		// ZERO it!
		SAFE_FILL_MEM (sensor, sizeof (pIvhSensorFusion), 0);
		TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for fusion sensor");
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
	sensor->sensor.type = IVH_SENSOR_TYPE_FUSION;
	sensor->sensor.d[1] = &sensor->output;

	for(int i = 0; i < number; i++)
	{
		sensors[i]->Attach(sensors[i], &(sensor->sensor));
	}

	sensor->AlgoNotification = AlgoNotification;

	return (pIvhSensor)(sensor);
}

void DestroySensorFusion(pIvhSensor me)
{
	pIvhSensorFusion sensor = (pIvhSensorFusion)(me);

	if(sensor != NULL)
	{
		SAFE_FREE_POOL(sensor, IVH_SENSOR_FUSION_POOL_TAG);
		sensor = NULL;
	}
}

//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------

static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
	ERROR_T retVal = ERROR_OK;
	SensorData data_event;

	pIvhSensorFusion sensor = (pIvhSensorFusion)(me);

	sensor->output.timeStampInMs = data->timeStampInMs;
	switch(type)
	{
	case IVH_SENSOR_TYPE_ACCELEROMETER3D:
		sensor->output.data.fusion.accl[0] = data->data.accel.xyz.x;
		sensor->output.data.fusion.accl[1] = data->data.accel.xyz.y;
		sensor->output.data.fusion.accl[2] = data->data.accel.xyz.z;
		if(sensor->AlgoNotification != NULL)
		{
			data_event.data[0] = data->data.accel.xyz.x;
			data_event.data[1] = data->data.accel.xyz.y;
			data_event.data[2] = data->data.accel.xyz.z;
			data_event.ts = data->timeStampInMs;
			sensor->AlgoNotification(IVH_VIRTUAL_SENSOR_ACCELEROMETER3D, &data_event);
		}
		TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]calibrated accel(%d,%d,%d)",data->data.accel.xyz.x,data->data.accel.xyz.y,data->data.accel.xyz.z);
		break;
	case IVH_SENSOR_TYPE_GYROSCOPE3D:
	case IVH_SENSOR_TYPE_GYROSCOPE_6AXIS:
		sensor->output.data.fusion.gyro[0] = data->data.gyro.xyz.x;
		sensor->output.data.fusion.gyro[1] = data->data.gyro.xyz.y;
		sensor->output.data.fusion.gyro[2] = data->data.gyro.xyz.z;
		if(sensor->AlgoNotification != NULL)
		{
			data_event.data[0] = data->data.gyro.xyz.x;
			data_event.data[1] = data->data.gyro.xyz.y;
			data_event.data[2] = data->data.gyro.xyz.z;
			data_event.ts = data->timeStampInMs;
			sensor->AlgoNotification(IVH_VIRTUAL_SENSOR_GYROSCOPE3D, &data_event);
		}
		TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]calibrated gyro(%d,%d,%d)",data->data.gyro.xyz.x,data->data.gyro.xyz.y,data->data.gyro.xyz.z);
		break;
	case IVH_SENSOR_TYPE_COMPASS3D:
		sensor->output.data.fusion.magn[0] = (int)(data->data.mag.xyzRaw.x);
		sensor->output.data.fusion.magn[1] = (int)(data->data.mag.xyzRaw.y);
		sensor->output.data.fusion.magn[2] = (int)(data->data.mag.xyzRaw.z);
		sensor->output.data.fusion.north = (int)(data->data.mag.compensatedHeadingMagneticNorth * 100.0f);
		sensor->output.data.fusion.error = (int)(data->data.mag.estimatedHeadingError);
		if(sensor->AlgoNotification != NULL)
		{
			data_event.data[0] = data->data.mag.xyzRaw.x;
			data_event.data[1] = data->data.mag.xyzRaw.y;
			data_event.data[2] = data->data.mag.xyzRaw.z;
			data_event.data[3] = (int)(data->data.mag.compensatedHeadingMagneticNorth * 100.0f);
			data_event.ts = data->timeStampInMs;
			sensor->AlgoNotification(IVH_VIRTUAL_SENSOR_COMPASS, &data_event);
		}
		TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]compass=%d, raw mag(%d,%d,%d)",sensor->output.data.fusion.north,
				sensor->output.data.fusion.magn[0],sensor->output.data.fusion.magn[1],sensor->output.data.fusion.magn[2]);
		break;
	case IVH_SENSOR_TYPE_INCLINOMETER:
		sensor->output.data.fusion.pitch = (int)(data->data.inclinometer.tilt.x*100.0f);
		sensor->output.data.fusion.roll = (int)(data->data.inclinometer.tilt.y*100.0f);
		sensor->output.data.fusion.yaw = (int)(data->data.inclinometer.tilt.z*100.0f);
		if(sensor->AlgoNotification != NULL)
		{
			data_event.data[0] = (int)(data->data.inclinometer.tilt.x*100.0f);
			data_event.data[1] = (int)(data->data.inclinometer.tilt.y*100.0f);
			data_event.data[2] = (int)(data->data.inclinometer.tilt.z*100.0f);
			data_event.ts = data->timeStampInMs;
			sensor->AlgoNotification(IVH_VIRTUAL_SENSOR_INCLINOMETER, &data_event);
		}
		TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]inclinometer(%d,%d,%d)",sensor->output.data.fusion.pitch,
				sensor->output.data.fusion.roll,sensor->output.data.fusion.yaw);
		break;
	case IVH_SENSOR_TYPE_ORIENTATION_6AXIS:
	case IVH_SENSOR_TYPE_ORIENTATION_9AXIS:
		sensor->output.data.fusion.rotation_matrix[0] = (int)(data->data.orientation.rotationMatrix.m11*10000.0f);
		sensor->output.data.fusion.rotation_matrix[1] = (int)(data->data.orientation.rotationMatrix.m12*10000.0f);
		sensor->output.data.fusion.rotation_matrix[2] = (int)(data->data.orientation.rotationMatrix.m13*10000.0f);
		sensor->output.data.fusion.rotation_matrix[3] = (int)(data->data.orientation.rotationMatrix.m21*10000.0f);
		sensor->output.data.fusion.rotation_matrix[4] = (int)(data->data.orientation.rotationMatrix.m22*10000.0f);
		sensor->output.data.fusion.rotation_matrix[5] = (int)(data->data.orientation.rotationMatrix.m23*10000.0f);
		sensor->output.data.fusion.rotation_matrix[6] = (int)(data->data.orientation.rotationMatrix.m31*10000.0f);
		sensor->output.data.fusion.rotation_matrix[7] = (int)(data->data.orientation.rotationMatrix.m32*10000.0f);
		sensor->output.data.fusion.rotation_matrix[8] = (int)(data->data.orientation.rotationMatrix.m33*10000.0f);
		sensor->output.data.fusion.quaternion[0] = (int)(data->data.orientation.quaternion.x*10000.0f);
		sensor->output.data.fusion.quaternion[1] = (int)(data->data.orientation.quaternion.y*10000.0f);
		sensor->output.data.fusion.quaternion[2] = (int)(data->data.orientation.quaternion.z*10000.0f);
		sensor->output.data.fusion.quaternion[3] = (int)(data->data.orientation.quaternion.w*10000.0f);
		if(sensor->AlgoNotification != NULL)
		{
			data_event.data[0] = (int)(data->data.orientation.rotationMatrix.m11*10000.0f);
			data_event.data[1] = (int)(data->data.orientation.rotationMatrix.m12*10000.0f);
			data_event.data[2] = (int)(data->data.orientation.rotationMatrix.m13*10000.0f);
			data_event.data[3] = (int)(data->data.orientation.rotationMatrix.m21*10000.0f);
			data_event.data[4] = (int)(data->data.orientation.rotationMatrix.m22*10000.0f);
			data_event.data[5] = (int)(data->data.orientation.rotationMatrix.m23*10000.0f);
			data_event.data[6] = (int)(data->data.orientation.rotationMatrix.m31*10000.0f);
			data_event.data[7] = (int)(data->data.orientation.rotationMatrix.m32*10000.0f);
			data_event.data[8] = (int)(data->data.orientation.rotationMatrix.m33*10000.0f);
			data_event.ts = data->timeStampInMs;
			sensor->AlgoNotification(IVH_VIRTUAL_SENSOR_ORIENTATION, &data_event);
		}
		break;
	case IVH_SENSOR_TYPE_AMBIENTLIGHT:
		sensor->output.data.fusion.als = data->data.alsMilliLux;
		TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]als = %d",sensor->output.data.fusion.als);
		break;
	case IVH_SENSOR_TYPE_THERMOMETER:
		sensor->output.data.fusion.temperature = data->data.temperatureMiniCentigrade;
		TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]temperature = %d",sensor->output.data.fusion.temperature);
		break;
	case IVH_SENSOR_TYPE_PROXIMITY:
		sensor->output.data.fusion.proximity = data->data.proximityMiniMeter;
		TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]proximity = %d",sensor->output.data.fusion.proximity);
		break;
	case IVH_SENSOR_TYPE_BAROMETER:
		sensor->output.data.fusion.baro = data->data.barometerData.pressure;
		TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]barometer = %d",sensor->output.data.fusion.baro);
		break;
	default:
		break;
	}

	return retVal;
}
