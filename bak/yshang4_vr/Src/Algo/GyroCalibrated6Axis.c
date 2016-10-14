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
#include "GyroCalibrated6Axis.h"
#include "platformCalibration.h"
#include "CalibrationCommon.h"
#include "stdlib.h"
#include "BackupRegisters.h"

#include "CompassCalibrated.h"
#include "SensorManagerTypes.h"

#include "Trace.h"

#define IVH_SENSOR_SENSORSCOPE6AXIS_POOL_TAG 'G6VI'
//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);

static void CalibrateData(const pIvhSensorGyroscope6Axis gyro, pIvhSensorData const pSensorData);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

pIvhSensor CreateSensorGyroscope6Axis(const pIvhSensor orientation)
{
    pIvhSensorGyroscope6Axis sensor = (pIvhSensorGyroscope6Axis)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorGyroscope6Axis), IVH_SENSOR_SENSORSCOPE6AXIS_POOL_TAG);

	if (sensor) 
	{
		// ZERO it!
		SAFE_FILL_MEM (sensor, sizeof (IvhSensorGyroscope6Axis), 0);
		TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for gyro6axis");
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
    sensor->sensor.type = IVH_SENSOR_TYPE_GYROSCOPE_6AXIS;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;
    orientation->Attach(orientation, &(sensor->sensor));

    sensor->index = 0;
    sensor->t0 = 0;

    return (pIvhSensor)(sensor);
}

void DestroySensorGyroscope6Axis(pIvhSensor me)
{
    pIvhSensorGyroscope6Axis sensor = (pIvhSensorGyroscope6Axis)(me);

    if(sensor != NULL)
    {
        SAFE_FREE_POOL(sensor, IVH_SENSOR_SENSORSCOPE6AXIS_POOL_TAG);
        sensor = NULL;
    }
}

//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------

static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
    ERROR_T retVal = ERROR_OK;

    if(type != IVH_SENSOR_TYPE_ORIENTATION_6AXIS)
    {
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]invalid sensor type, expect [IVH_SENSOR_TYPE_ORIENTATION_6AXIS]");
    }
    SAFE_MEMCPY(me->d[0], data);

    Calibrate(me);
    return retVal;
}

static ERROR_T Calibrate(IvhSensor* me)
{
    ERROR_T retVal = ERROR_OK;
    IvhSensorData temp;
    pIvhSensorGyroscope6Axis sensor = (pIvhSensorGyroscope6Axis)(me);

    SAFE_MEMCPY(&temp, &sensor->input);
    CalibrateData(sensor, &temp);
    SAFE_MEMCPY(&sensor->output, &temp);

    me->Notify(me);
    return retVal;
}

static void CalibrateData(const pIvhSensorGyroscope6Axis gyro, pIvhSensorData const pSensorData)
{
    //TODO: the first value may out of range, but output value will be normal after severial samples...need do further test
    //use the first value as the initlial value will fix this issue
    FLOAT_VECTOR9_T* rm0 = &(gyro->rm0);
    /* init value is
    {
        pSensorData->data.orientation.rotationMatrix.m11,pSensorData->data.orientation.rotationMatrix.m21,pSensorData->data.orientation.rotationMatrix.m31,
        pSensorData->data.orientation.rotationMatrix.m12,pSensorData->data.orientation.rotationMatrix.m22,pSensorData->data.orientation.rotationMatrix.m32,
        pSensorData->data.orientation.rotationMatrix.m13,pSensorData->data.orientation.rotationMatrix.m23,pSensorData->data.orientation.rotationMatrix.m33
    };*/

    FLOAT_VECTOR9_T rm1={
        pSensorData->data.orientation.rotationMatrix.m11,pSensorData->data.orientation.rotationMatrix.m12,pSensorData->data.orientation.rotationMatrix.m13,
        pSensorData->data.orientation.rotationMatrix.m21,pSensorData->data.orientation.rotationMatrix.m22,pSensorData->data.orientation.rotationMatrix.m23,
        pSensorData->data.orientation.rotationMatrix.m31,pSensorData->data.orientation.rotationMatrix.m32,pSensorData->data.orientation.rotationMatrix.m33
    };
    uint32_t t1=pSensorData->timeStampInMs;
    float deltat=(float)(t1-gyro->t0)/100000.0f;

    if(deltat < .001f) //1ms
    {
        //ignore...
        return;
    }
    FLOAT_VECTOR9_T d={//why?
        rm0->m11*rm1.m11+rm0->m12*rm1.m21+rm0->m13*rm1.m31,rm0->m11*rm1.m12+rm0->m12*rm1.m22+rm0->m13*rm1.m32,rm0->m11*rm1.m13+rm0->m12*rm1.m23+rm0->m13*rm1.m33,
        rm0->m21*rm1.m11+rm0->m22*rm1.m21+rm0->m23*rm1.m31,rm0->m21*rm1.m12+rm0->m22*rm1.m22+rm0->m23*rm1.m32,rm0->m21*rm1.m13+rm0->m22*rm1.m23+rm0->m23*rm1.m33,
        rm0->m31*rm1.m11+rm0->m32*rm1.m21+rm0->m33*rm1.m31,rm0->m31*rm1.m12+rm0->m32*rm1.m22+rm0->m33*rm1.m32,rm0->m31*rm1.m13+rm0->m32*rm1.m23+rm0->m33*rm1.m33
    };
    //store last data
    rm0->m11=rm1.m11,rm0->m12=rm1.m21,rm0->m13=rm1.m31;
    rm0->m21=rm1.m12,rm0->m22=rm1.m22,rm0->m23=rm1.m32;
    rm0->m31=rm1.m13,rm0->m32=rm1.m23,rm0->m33=rm1.m33;
    gyro->t0=t1;
    //unit!
    d.m12/=d.m11,d.m13/=d.m11;
    d.m21/=d.m22,d.m23/=d.m22;
    d.m31/=d.m33,d.m32/=d.m33;

    // we need select a delta theta according to current device oritation state to improve accuracy
    //as a simple algorithm, we just use sigma-average...
    //update: sigma-average algo is not a good idea according to the test result on P0 board.(bug832)
    /*float omiga[]={(d.m32-d.m23)/(2.0f*deltat),(d.m13-d.m31)/(2.0f*deltat),(d.m21-d.m12)/(2.0f*deltat)};*/
    //check gimble lock...
    float omiga[]={fabs(d.m32)>fabs(d.m23)?-d.m23/deltat:d.m32/deltat,fabs(d.m13)>fabs(d.m31)?-d.m31/deltat:d.m13/deltat,fabs(d.m21)>fabs(d.m12)?-d.m12/deltat:d.m21/deltat};

    //TODO: should apply tilt compensition and fast rotate compensition to improve accuracy

    int* index = &(gyro->index);
    float current_data[3]={omiga[0]*RAD_TO_DEG,omiga[1]*RAD_TO_DEG,omiga[2]*RAD_TO_DEG};

    gyro->last_gyro[*index][0]=current_data[0];
    gyro->last_gyro[*index][1]=current_data[1];
    gyro->last_gyro[*index][2]=current_data[2];
    ++*index;
    (*index)%=5;

    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]current(%d,%d,%d)",(int)(current_data[0]),(int)(current_data[1]),(int)(current_data[2]));
    /*
    for(int j=0;j<3;j++)
    {
        int min=(gyro->last_gyro[0][j]<gyro->last_gyro[1][j]?gyro->last_gyro[0][j]:gyro->last_gyro[1][j])<gyro->last_gyro[2][j]?(gyro->last_gyro[0][j]<gyro->last_gyro[1][j]?0:1):2;
        current_data[j]=gyro->last_gyro[(min+1)%3][j]<gyro->last_gyro[(min+2)%3][j]?gyro->last_gyro[(min+1)%3][j]:gyro->last_gyro[(min+2)%3][j];
    }*/
    //arithmetic mean filter
    for(int j=0;j<3;j++)
    {
        float sum = 0;
        for(int i = 0; i<5; i++)
        {
            sum+=gyro->last_gyro[i][j];
        }
        current_data[j] = sum/=5;
    }
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]current2(%d,%d,%d)",(int)(current_data[0]),(int)(current_data[1]),(int)(current_data[2]));

    //convert from phicial rotation to human-sense rotation
    for(int i = 0; i< 3; i++)
    {
        if(fabs(current_data[i])<10.f)
        {
            float t = current_data[i]/fabs(current_data[i]);
            current_data[i]*=current_data[i];
            current_data[i]/=t*10.0f;
        }
    }

    int32_t out_data[]={(int32_t)(current_data[0]*1000.0f),(int32_t)(current_data[1]*1000.0f),(int32_t)(current_data[2]*1000.0f)};
    int32_t* last_data = gyro->last_data;// init valuse is {out_data[0],out_data[1],out_data[2]};
    last_data[0]=(last_data[0]*4+out_data[0]*6)/10;
    last_data[1]=(last_data[1]*4+out_data[1]*6)/10;
    last_data[2]=(last_data[2]*4+out_data[2]*6)/10;

    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]current(%d,%d,%d)last(%d,%d,%d)",(int)(current_data[0]),(int)(current_data[1]),
        (int)(current_data[2]),(int)(last_data[0]*10),(int)(last_data[1]*10),(int)(last_data[2]*10));

    pSensorData->data.gyro.xyz.x = last_data[0];
    pSensorData->data.gyro.xyz.y = last_data[1];
    pSensorData->data.gyro.xyz.z = last_data[2];
    pSensorData->timeStampInMs = t1;

    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]delta(10000,%d,%d),(%d,10000,%d),(%d,%d,10000)",(int)(d.m12*10000),(int)(d.m13*10000),
        (int)(d.m21*10000),(int)(d.m23*10000),(int)(d.m31*10000),(int)(d.m32*10000));
}
