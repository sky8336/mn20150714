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
// AlgorithmManager.h : algo interface

#ifndef ALGORITHMMANAGER_H
#define ALGORITHMMANAGER_H

#include "AccelerometerCalibrated.h"
#include "CompassCalibrated.h"
#include "AlsCalibrated.h"
#include "GyroCalibrated.h"
#include "GyroCalibrated6Axis.h"
#include "DeviceOrientation.h"
#include "CompassOrientation.h"
#include "Inclinometer.h"
#include "BiometricProximity.h"
#include "Algo/Temperature.h"
#include "Barometer.h"
#include "platformCalibration.h"

#include "Fusion.h"
#include "Dsh_ipc.h"

/* physical sensor definitaion, pass to Algo_Init as argument */
enum 
{
    IVH_PHYSICAL_SENSOR_ACCELEROMETER3D = (1UL),
    IVH_PHYSICAL_SENSOR_GYROSCOPE3D = (1UL<<1),
    IVH_PHYSICAL_SENSOR_MAGNETOMETER3D = (1UL<<2),
    IVH_PHYSICAL_SENSOR_AMBIENTLIGHT = (1UL<<3),
    IVH_PHYSICAL_SENSOR_HYGROMETER = (1UL<<4),
    IVH_PHYSICAL_SENSOR_THERMOMETER = (1UL<<5),
    IVH_PHYSICAL_SENSOR_BAROMETER = (1UL<<6),
    IVH_PHYSICAL_SENSOR_PROXIMITY = (1UL<<7),
};

/* virtual sensor definitaion, return from Algo_Init for upper layer use */
enum 
{
    IVH_VIRTUAL_SENSOR_ACCELEROMETER3D = (1UL),
    IVH_VIRTUAL_SENSOR_GYROSCOPE3D = (1UL<<1),
    IVH_VIRTUAL_SENSOR_COMPASS = (1UL<<2),
    IVH_VIRTUAL_SENSOR_INCLINOMETER = (1UL<<3),
    IVH_VIRTUAL_SENSOR_ORIENTATION = (1UL<<4),
    IVH_VIRTUAL_SENSOR_AMBIENTLIGHT = (1UL<<5),
    IVH_VIRTUAL_SENSOR_HYGROMETER = (1UL<<6),
    IVH_VIRTUAL_SENSOR_THERMOMETER = (1UL<<7),
    IVH_VIRTUAL_SENSOR_BAROMETER = (1UL<<8),
    IVH_VIRTUAL_SENSOR_PROXIMITY = (1UL<<9),
};

typedef struct _IVH_ALGO
{
    ULONG calibrated;
    pIvhPlatform platform;
    pIvhSensor accelerometer;
    pIvhSensor gyrometer;
    pIvhSensor gyrometer6axis;
    pIvhSensor magnetometer;
    pIvhSensor ambientlight;
    pIvhSensor compass;
    pIvhSensor inclinometer;
    pIvhSensor orientation;
    pIvhSensor proximity;
    pIvhSensor thermometer;
    pIvhSensor barometer;
    pIvhSensor fusion;
} IvhAlgo, *pIvhAlgo;

typedef enum __SENSOR_ACCURACY{
    eSENSOR_ACCURACY_LOW = 1,
    eSENSOR_ACCURACY_MEDIUM,
    eSENSOR_ACCURACY_HIGH,
} SENSOR_ACCURACY;

typedef struct _SENSOR_DATA
{
    unsigned long ts;
    int data[9];
} SensorData, *pSensorData;

pIvhAlgo Alg_Init(ALGO_NOTIFICATION AlgoNotication, ULONG sensori, ULONG* sensoro);
void Alg_DeInit(pIvhAlgo algo);

void Alg_UpdateGyrometerData(pIvhAlgo algo, int GyrX, int GyrY, int GyrZ, int ts);
void Alg_UpdateAccelerometerData(pIvhAlgo algo, int AccX, int AccY, int AccZ, int ts);
void Alg_UpdateMagnetometerData(pIvhAlgo algo, int MagX, int MagY, int MagZ, int ts);
void Alg_UpdateAmbientlightData(pIvhAlgo algo, unsigned als);
void Alg_UpdateProximityData(pIvhAlgo algo, int sar);
void Alg_UpdateTemperatureData(pIvhAlgo algo, int tem);
void Alg_UpdateBarometerData(pIvhAlgo algo, int baro);

void Alg_GetEuler(pIvhAlgo algo, int ypr[3]);
void Alg_GetQuaternion(pIvhAlgo algo, int q[4]);
void Alg_GetOrientationMatrix(pIvhAlgo algo, struct rot_raw_data* rot);
void Alg_GetGyrometer(pIvhAlgo algo, struct gyro_raw_data *gyro);
void Alg_GetMagnetometer(pIvhAlgo algo, struct compass_raw_data *magn);
void Alg_GetAccelerometer(pIvhAlgo algo, struct accel_data *acc);
int  Alg_GetNorth(pIvhAlgo algo);
BOOL Alg_DetectShake(pIvhAlgo algo);
void Alg_GetAmbientlightCurve(pIvhAlgo algo, unsigned short curve[20]);
void Alg_GetAmbientlightData(pIvhAlgo algo, unsigned *als);
void Alg_GetAccuracy(pIvhAlgo algo, SENSOR_ACCURACY* accuracy);
void Alg_GetProximity(pIvhAlgo algo, int* sar);
void Alg_GetTemperature(pIvhAlgo algo, int* tem);
void Alg_GetBarometer(pIvhAlgo algo, int* baro);

#endif //ALGORITHMMANAGER_H
