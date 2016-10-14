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
//
// Sensor.h : define sensor interface
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __IVH_SENSOR_H__
#define __IVH_SENSOR_H__

#include "SensorManagerTypes.h"
#include "Common.h"
#include "Error.h"
#include "platformCalibration.h"

#define MAX_SENSOR_DEPENDENCY 5

typedef enum {
	IVH_SENSOR_TYPE_NONE = 0,
	//physical sensor
	IVH_SENSOR_TYPE_ACCELEROMETER3D = 1,
	IVH_SENSOR_TYPE_GYROSCOPE3D,
	IVH_SENSOR_TYPE_MAGNETOMETER3D,
	IVH_SENSOR_TYPE_AMBIENTLIGHT,
	IVH_SENSOR_TYPE_HYGROMETER,
	IVH_SENSOR_TYPE_THERMOMETER,
	IVH_SENSOR_TYPE_BAROMETER,
	IVH_SENSOR_TYPE_PROXIMITY,

	//virtual sensor
	IVH_SENSOR_TYPE_GYROSCOPE_6AXIS,
	IVH_SENSOR_TYPE_COMPASS3D,
	IVH_SENSOR_TYPE_ORIENTATION_6AXIS,
	IVH_SENSOR_TYPE_ORIENTATION_9AXIS,
	IVH_SENSOR_TYPE_INCLINOMETER,

	//fusion sensor, keep me in the last!
	IVH_SENSOR_TYPE_FUSION,
}IvhSensorType;

typedef struct _IVH_SENSOR_ IvhSensor;
typedef struct _IVH_SENSOR_
{
	int numbers;
	IvhSensor* o[MAX_SENSOR_DEPENDENCY];
	IvhSensorData* d[2];
	IvhSensorType type;

	ERROR_T (*Attach)(IvhSensor* me, IvhSensor* ob);
	ERROR_T (*Notify)(IvhSensor* me);
	ERROR_T (*Update)(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
	pIvhSensorData (*QueryData)(IvhSensor* me);
}*pIvhSensor;

ERROR_T SensorAttach(IvhSensor* me, IvhSensor* ob);
ERROR_T SensorNotify(IvhSensor* me);
pIvhSensorData SensorQueryData(IvhSensor* me);

#endif //#define __IVH_SENSOR_H__
