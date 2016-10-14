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
// AlgorithmManager.cpp : algo interface

#include "AlgorithmManager.h"
#include "Trace.h"
#include "CalibrationCommon.h"
#include "Fusion.h"


#define IVH_SENSOR_ALGO_POOL_TAG 'OGLA'

static inline pIvhSensorData Alg_GetFusionData(pIvhAlgo algo);

pIvhAlgo Alg_Init(ALGO_NOTIFICATION AlgoNotication, ULONG sensori, ULONG* sensoro)
{
    TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "%!FUNC! Entry");

    pIvhAlgo algo = (pIvhAlgo)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhAlgo), IVH_SENSOR_ALGO_POOL_TAG);

    if (algo) 
    {
        // ZERO it!
        SAFE_FILL_MEM (algo, sizeof (IvhAlgo), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for algo");
    }
    else 
    {
        //  No memory
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "not enough memory");
        return NULL;
    }

    ///algo->dev = dev;
    //cjy...BackupRegistersInit(algo->dev);
    algo->platform = CreatePlatform(&algo->calibrated);

    ULONG ability = 0;

    int number = 0;
    pIvhSensor sensors[IVH_SENSOR_TYPE_FUSION];
    if(sensori & IVH_PHYSICAL_SENSOR_ACCELEROMETER3D)
    {
        sensors[number] = algo->accelerometer = CreateSensorAccelerometer(algo->platform);
        if(sensors[number] != NULL)
        {
            number++;
            ability |= IVH_VIRTUAL_SENSOR_ACCELEROMETER3D;
        }
    }
    if(sensori & IVH_PHYSICAL_SENSOR_GYROSCOPE3D)
    {
        sensors[number] = algo->gyrometer = CreateSensorGyroscope(algo->platform);
        if(sensors[number] != NULL)
        {
            number++;
            ability |= IVH_VIRTUAL_SENSOR_GYROSCOPE3D;
        }
    }
    if((sensori & IVH_PHYSICAL_SENSOR_ACCELEROMETER3D) && (sensori & IVH_PHYSICAL_SENSOR_MAGNETOMETER3D))
    {
        algo->magnetometer = CreateSensorMagnetometer(algo->platform, algo->accelerometer, algo->gyrometer);
        if(algo->magnetometer != NULL)
        {
            sensors[number] = algo->orientation = CreateSensorOrientation(algo->platform, algo->accelerometer, algo->gyrometer, algo->magnetometer);
            if(sensors[number] != NULL)
            {
                number++;
                ability |= IVH_VIRTUAL_SENSOR_ORIENTATION;
            }
        }
    }
    if(algo->orientation != NULL)
    {
        //create inclimeter and compass sensor
        sensors[number] = algo->compass = CreateSensorCompass(algo->orientation);
        if(sensors[number] != NULL)
        {
            number++;
            ability |= IVH_VIRTUAL_SENSOR_COMPASS;
        }

        sensors[number] = algo->inclinometer = CreateSensorInclinometer(algo->orientation);
        if(sensors[number] != NULL)
        {
            number++;
            ability |= IVH_VIRTUAL_SENSOR_INCLINOMETER;
        }
        //create virtual gyroscope if physical gyroscope is not exist
        if((sensori & IVH_PHYSICAL_SENSOR_GYROSCOPE3D) == 0)
        {
            sensors[number] = algo->gyrometer6axis = CreateSensorGyroscope6Axis(algo->orientation);
            if(sensors[number] != NULL)
            {
                number++;
                ability |= IVH_VIRTUAL_SENSOR_GYROSCOPE3D;
            }
        }
    }
    if(sensori & IVH_PHYSICAL_SENSOR_AMBIENTLIGHT)
    {
        sensors[number] = algo->ambientlight = CreateSensorAmbientlight(algo->platform);
        if(sensors[number] != NULL)
        {
            number++;
            ability |= IVH_VIRTUAL_SENSOR_AMBIENTLIGHT;
        }
    }
    if(sensori & IVH_PHYSICAL_SENSOR_PROXIMITY)
    {
        sensors[number] = algo->proximity = CreateSensorBiometricProximity(algo->platform);
        if(sensors[number] != NULL)
        {
            number++;
            ability |= IVH_VIRTUAL_SENSOR_PROXIMITY;
        }
    }
    if(sensori & IVH_PHYSICAL_SENSOR_THERMOMETER)
    {
        sensors[number] = algo->thermometer = CreateSensorTemperature(algo->platform);
        if(sensors[number] != NULL)
        {
            number++;
            ability |= IVH_VIRTUAL_SENSOR_THERMOMETER;
        }
    }
    if(sensori & IVH_PHYSICAL_SENSOR_BAROMETER)
    {
        sensors[number] = algo->barometer = CreateSensorBarometer(algo->platform);
        if(sensors[number] != NULL)
        {
            number++;
            ability |= IVH_VIRTUAL_SENSOR_BAROMETER;
        }
    }
    algo->fusion = CreateSensorFusion(AlgoNotication, sensors, number);

    if(*sensoro == 0)
    {
        *sensoro = ability;
    }
    else
    {
        *sensoro &= ability;
    }

    TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "%!FUNC! Exit");
    return algo;
}

void Alg_DeInit(pIvhAlgo algo)
{
    TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "%!FUNC! Entry");
    if(algo->accelerometer != NULL)
    {
        DestroySensorAccelerometer(algo->accelerometer);
        algo->accelerometer = NULL;
    }
    if(algo->gyrometer != NULL)
    {
        DestroySensorGyroscope(algo->gyrometer);
        algo->gyrometer = NULL;
    }
    if(algo->magnetometer != NULL)
    {
        DestroySensorMagnetometer(algo->magnetometer);
        algo->magnetometer = NULL;
    }
    if(algo->orientation != NULL)
    {
        DestroySensorOrientation(algo->orientation);
        algo->orientation = NULL;
    }
    if(algo->compass != NULL)
    {
        DestroySensorCompass(algo->compass);
        algo->compass = NULL;
    }
    if(algo->inclinometer != NULL)
    {
        DestroySensorInclinometer(algo->inclinometer);
        algo->inclinometer = NULL;
    }
    if(algo->gyrometer6axis != NULL)
    {
        DestroySensorGyroscope6Axis(algo->gyrometer6axis);
        algo->gyrometer6axis = NULL;
    }
    if(algo->ambientlight != NULL)
    {
        DestroySensorAmbientlight(algo->ambientlight);
        algo->ambientlight = NULL;
    }
    if(algo->proximity != NULL)
    {
        DestroySensorBiometricProximity(algo->proximity);
        algo->proximity = NULL;
    }
    if(algo->thermometer != NULL)
    {
        DestroySensorTemperature(algo->thermometer);
        algo->thermometer = NULL;
    }
    if(algo->barometer != NULL)
    {
        DestroySensorBarometer(algo->barometer);
        algo->barometer = NULL;
    }
    if(algo->fusion != NULL)
    {
        DestroySensorFusion(algo->fusion);
        algo->fusion = NULL;
    }

    if(algo->platform != NULL)
    {
        DestroyPlatform(algo->platform);
        algo->platform = NULL;
    }
    //cjy...BackupRegistersDeInit(algo->dev);
    if(algo != NULL)
    {
        SAFE_FREE_POOL(algo, IVH_SENSOR_ALGO_POOL_TAG);
        algo = NULL;
    }
    TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "%!FUNC! Exit");
}

void Alg_UpdateGyrometerData(pIvhAlgo algo, int GyrX, int GyrY, int GyrZ, int ts)
{
    IvhSensorData data;
    data.timeStampInMs = ts;
    data.data.gyro.xyz.x = GyrX;
    data.data.gyro.xyz.y = GyrY;
    data.data.gyro.xyz.z = GyrZ;

    if(algo->gyrometer != NULL)
    {
        algo->gyrometer->Update(algo->gyrometer, &data, IVH_SENSOR_TYPE_GYROSCOPE3D);
    }
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]gyro(%d,%d,%d)",data.data.gyro.xyz.x,data.data.gyro.xyz.y,data.data.gyro.xyz.z);
}

void Alg_UpdateAccelerometerData(pIvhAlgo algo, int AccX, int AccY, int AccZ, int ts)
{
    IvhSensorData data;
    data.timeStampInMs = ts;
    data.data.accel.xyz.x = AccX;
    data.data.accel.xyz.y = AccY;
    data.data.accel.xyz.z = AccZ;

    if(algo->accelerometer != NULL)
    {
        algo->accelerometer->Update(algo->accelerometer, &data, IVH_SENSOR_TYPE_ACCELEROMETER3D);
    }
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]accel(%d,%d,%d)",data.data.accel.xyz.x,data.data.accel.xyz.y,data.data.accel.xyz.z);
}

void Alg_UpdateMagnetometerData(pIvhAlgo algo, int MagX, int MagY, int MagZ, int ts)
{
    IvhSensorData data;
    data.timeStampInMs = ts;
    data.data.mag.xyzRaw.x = MagX;
    data.data.mag.xyzRaw.y = MagY;
    data.data.mag.xyzRaw.z = MagZ;

    if(algo->magnetometer != NULL)
    {
        algo->magnetometer->Update(algo->magnetometer, &data, IVH_SENSOR_TYPE_MAGNETOMETER3D);
    }
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]mag(%d,%d,%d)",data.data.mag.xyzRaw.x,data.data.mag.xyzRaw.y,data.data.mag.xyzRaw.z);
}

void Alg_UpdateAmbientlightData(pIvhAlgo algo, unsigned als)
{
    IvhSensorData data;
    data.data.alsMilliLux = als;

    if(algo->ambientlight != NULL)
    {
        algo->ambientlight->Update(algo->ambientlight, &data, IVH_SENSOR_TYPE_AMBIENTLIGHT);
    }
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]als = %d",als);
}

void Alg_GetEuler(pIvhAlgo algo, int ypr[3])
{
    pIvhSensorData d = Alg_GetFusionData(algo);
    ypr[0] = d->data.fusion.yaw;
    ypr[1] = d->data.fusion.pitch;
    ypr[2] = d->data.fusion.roll;
}

int Alg_GetNorth(pIvhAlgo algo)
{
    pIvhSensorData d = Alg_GetFusionData(algo);
    return d->data.fusion.north;
}

void Alg_GetQuaternion(pIvhAlgo algo, int q[4])
{
    pIvhSensorData d = Alg_GetFusionData(algo);
    q[0] = d->data.fusion.quaternion[0];
    q[1] = d->data.fusion.quaternion[1];
    q[2] = d->data.fusion.quaternion[2];
    q[3] = d->data.fusion.quaternion[3];
}

void Alg_GetOrientationMatrix(pIvhAlgo algo, struct rot_raw_data* rot)
{
    pIvhSensorData d = Alg_GetFusionData(algo);
    for(int i = 0; i < 9; i++)
    {
        rot->matrix[i] = d->data.fusion.rotation_matrix[i];
    }
	rot->ts = d->timeStampInMs;
}

void Alg_GetGyrometer(pIvhAlgo algo, struct gyro_raw_data *gyro)
{
    pIvhSensorData d = Alg_GetFusionData(algo);
	
    gyro->x = d->data.fusion.gyro[0];
	gyro->y = d->data.fusion.gyro[1];
	gyro->z = d->data.fusion.gyro[2];
	gyro->ts = d->timeStampInMs;
}

void Alg_GetMagnetometer(pIvhAlgo algo, struct compass_raw_data *magn)
{
    pIvhSensorData d = Alg_GetFusionData(algo);

	magn->x = d->data.fusion.magn[0];
	magn->y = d->data.fusion.magn[1];
	magn->z = d->data.fusion.magn[2];
	magn->ts = d->timeStampInMs;
}

void Alg_GetAccelerometer(pIvhAlgo algo, struct accel_data *acc)
{
    pIvhSensorData d = Alg_GetFusionData(algo);

	acc->x = d->data.fusion.accl[0];
	acc->y = d->data.fusion.accl[1];
	acc->z = d->data.fusion.accl[2];
	acc->ts = d->timeStampInMs;
}

void Alg_GetAmbientlightData(pIvhAlgo algo, unsigned *als)
{
    pIvhSensorData d = Alg_GetFusionData(algo);
    *als = d->data.fusion.als;
}

void Alg_GetAmbientlightCurve(pIvhAlgo algo, unsigned short curve[20])
{
    UNREFERENCED_PARAMETER(algo);
    static const unsigned short als_curve[20]={12,0,24,0,35,1,47,4,59,10,71,28,82,75,94,206,106,565,118,1547};
    for(int i = 0; i < 20; i++)
    {
        curve[i] = als_curve[i];
    }
}

void Alg_GetAccuracy(pIvhAlgo algo, SENSOR_ACCURACY* accuracy)
{
    if(algo->calibrated == 0)
    {
        *accuracy = eSENSOR_ACCURACY_LOW;
    }
    else if(algo->calibrated == 1) // per system
    {
        *accuracy = eSENSOR_ACCURACY_HIGH;
    }
    else // per model
    {
        pIvhSensorData d = Alg_GetFusionData(algo);
        if(d->data.fusion.error < 10)
        {
            *accuracy = eSENSOR_ACCURACY_HIGH;
        }
        else if(d->data.fusion.error <30)
        {
            *accuracy = eSENSOR_ACCURACY_MEDIUM;
        }
        else
        {
            *accuracy = eSENSOR_ACCURACY_LOW;
        }
    }
}

BOOL Alg_DetectShake(pIvhAlgo algo)
{
    UNREFERENCED_PARAMETER(algo);
    return FALSE;
}

static inline pIvhSensorData Alg_GetFusionData(pIvhAlgo algo)
{
    return algo->fusion->QueryData(algo->fusion);
}

void Alg_UpdateProximityData(pIvhAlgo algo, int proximity)
{
    IvhSensorData data;
    data.data.proximityMiniMeter = proximity;

    if(algo->proximity)
    {
        algo->proximity->Update(algo->proximity, &data, IVH_SENSOR_TYPE_PROXIMITY);
    }
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]proximity = %d",proximity);
}

void Alg_GetProximity(pIvhAlgo algo, int* proximity)
{
    pIvhSensorData d = Alg_GetFusionData(algo);
    *proximity = d->data.fusion.proximity;
}

void Alg_UpdateTemperatureData(pIvhAlgo algo, int temperature)
{
    IvhSensorData data;
    data.data.temperatureMiniCentigrade = temperature;

    if(algo->thermometer)
    {
        algo->thermometer->Update(algo->thermometer, &data, IVH_SENSOR_TYPE_THERMOMETER);
    }
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]temperature = %d",temperature);
}

void Alg_GetTemperature(pIvhAlgo algo, int* temperature)
{
    pIvhSensorData d = Alg_GetFusionData(algo);
    *temperature = d->data.fusion.temperature;
}

void Alg_UpdateBarometerData(pIvhAlgo algo, int barometer)
{
    IvhSensorData data;
	data.data.barometerData.pressure = barometer;

    if(algo->barometer)
    {
        algo->barometer->Update(algo->barometer, &data, IVH_SENSOR_TYPE_BAROMETER);
    }
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "[%!FUNC!]barometer = %d",barometer);
}

void Alg_GetBarometer(pIvhAlgo algo, int* barometer)
{
    pIvhSensorData d = Alg_GetFusionData(algo);
    *barometer = d->data.fusion.baro;
}
