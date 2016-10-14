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
////////////////////////////////////////////////////////////////////////////////
//
// SensorManagerTypes.h :
//
#if !defined(_SENSOR_MANAGER_TYPES_H_)
#define _SENSOR_MANAGER_TYPES_H_

#include <stdint.h>
#include "Error.h"
#include "Common.h"
#include "Vector.h"

#define COVARIANCE_MATRIX_SIZE 6
// A union of structs representing sensor data along with
// the timestamp when the data was acquired/calculated.
typedef struct _IVH_SENSOR_DATA_T
{
	uint32_t        timeStampInMs;
	union
	{
		uint32_t alsMilliLux;
		uint32_t proximityMiniMeter;
		uint32_t temperatureMiniCentigrade;
		struct
		{
			FLOAT_VECTOR4_T quaternion;
			FLOAT_VECTOR9_T rotationMatrix;
			float           yawYAxis;
			float           yawZAxis;
			float           pitch;
			float           roll;
			float           rollZAxis;
			INT32_VECTOR3_T magRaw;
			uint8_t         estimatedHeadingError;
		}  orientation;
		struct
		{
			INT32_VECTOR3_T xyz;
		} accel;
		struct
		{
			INT32_VECTOR3_T xyz;
		} gyro;
		struct
		{
			INT32_VECTOR3_T xyzRaw;
			FLOAT_VECTOR3_T xyzCalibrated;
			FLOAT_VECTOR3_T xyzRotated;
			float           covariance[COVARIANCE_MATRIX_SIZE];
			float           compensatedHeadingMagneticNorth;
			uint8_t         estimatedHeadingError;
		} mag;
		struct
		{
			FLOAT_VECTOR3_T tilt;
			uint8_t         estimatedHeadingError;
		} inclinometer;
		struct
		{
			INT32_VECTOR3_T xyz;
			float           lidangle;
			float           baseangle;
		} protractor;
		struct
		{
			float           humidity;
			float           temperature;
		} hygrometer;
		struct {
			BOOLEAN humanPresence;
			uint32_t customValue1;
			uint32_t customValue2;
		} humanPresenceData;
		struct {
			uint8_t motionValue;
		} motionSensorData;
		struct {
			uint32_t pressure;
		} barometerData;
		struct {
			INT32_VECTOR3_T xyzLid;
			INT32_VECTOR3_T xyzBase;
			INT32_VECTOR3_T xyzLidClosedMag;
			float lidAngleFromLevel;
			float baseAngleFromLevel;
			float lidToBaseAngle;
			uint8_t lidState;
		} lidMonitor;
		struct {
			int yaw; //Euler Angles
			int pitch;
			int roll;
			int rotation_matrix[9]; //rotation matrix
			int quaternion[4]; //quaternion
			int gyro[3]; //calibrated data for output
			int accl[3]; //calibrated data for output
			int magn[3]; //calibrated data for output
			int als_curve[20];
			unsigned int als;
			int als_multiplier;
			int north;
			int error;
			int proximity;
			int temperature;
			int baro;
		} fusion;
		BYTE bUnknown;
	} data;
} IvhSensorData, *pIvhSensorData;

#endif /* !defined(_SENSOR_MANAGER_TYPES_H_) */


