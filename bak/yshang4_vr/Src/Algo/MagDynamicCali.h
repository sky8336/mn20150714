
////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2011, Intel Corporation.  All Rights Reserved.	      //
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

#if !defined(_MAG_DYNAMIC_CALI_H_)
#define _MAG_DYNAMIC_CALI_H

#include "SensorManagerTypes.h"
#include "Rotation.h"
#include "Matrix.h"
#include "platformCalibration.h"

#ifndef __GOD_BLESS_ME__ //God bless me!
#define __GOD_BLESS_ME__
//operational modes of CompassCalibrated
typedef enum
{
    CALIBRATION_STATE_INIT,
    CALIBRATION_STATE_RUNNING
} CALIBRATION_STATE_T;

typedef struct
{
    MATRIX_STRUCT_T a;
    MATRIX_STRUCT_T b;
} MAG_CALIBRATION_3D_PARAMS_STRUCT_T;

// Define a bitfield to store one or more planes for each point
typedef uint8_t PLANE_T;

enum
{
    NORTH,
    EAST,
    SOUTH,
    WEST,
    NUM_CARDINALS
};

// The calibration DFU contains offsets for the following orientations
// but only ZUP and ZDN are currently used.
enum
{
    ZUP,
    ZDN,
    XUP,
    XDN,
    YUP,
    YDN,
    NUM_ORIENTATIONS
};

typedef struct _MAGN_DYNAMICCALI_T
{
    CALIBRATION_STATE_T calibrationState; 

    MAG_CALIBRATION_3D_PARAMS_STRUCT_T magParams3D;

    // Current anomaly state
    BOOLEAN inAnomaly;

    // 2D array of heading corrections to apply in different orientations
    int16_t correctionAtCardinals[NUM_ORIENTATIONS][NUM_CARDINALS];

    PLANE_T PLANE_NONE;
    PLANE_T PLANE_XY;
    PLANE_T PLANE_XZ;
    PLANE_T PLANE_YZ;

    uint32_t anomalyHoldCount;

    //for RefineMagOffsets
    float magPrev[3];
    float prevMagNorm;

    ROTATION_VECTOR_T s_prevCalibratedMag[NUM_AXES];
    ROTATION_VECTOR_T s_prevRotatedMag[NUM_AXES];
    ROTATION_MATRIX_T rmat[ROTATION_MATRIX_NUM_ELEMS];

    //for MagDetectAnomaly
    float lastNorm;
    uint32_t lastTime;
    uint32_t sampleCount;
    BOOLEAN lastAnomalyState;
    float averageNorm;
    float prevNorm;

    pIvhPlatform Platform;
    //new data struct
    ROTATION_MATRIX_T EqualizationArray[20];
    ROTATION_VECTOR_T* Fifo[NUM_AXES]; // x,y,z data fifo
    int FifoIndex[NUM_AXES]; //x,y,z data fifo index
    int FifoSize[NUM_AXES]; //x,y,z data fifo size
    unsigned FifoSamples[NUM_AXES]; //x,y,z data fifo size

    //for error estimate
    ROTATION_VECTOR_T ErrorFifo[NUM_AXES*6]; // max 6 sample
    int ErrorFifoHead; //error fifo head
    int ErrorFifoEnd; //error fifo end
    unsigned ErrorFifoSamples; //error fifo total samples
    int Error; //estimate error (in degrees)
}*PMAGN_DYNAMICCALI_T;
#endif

void MagDynamicCali(IN PMAGN_DYNAMICCALI_T cali,
                    INOUT IvhSensorData* pSensorData,
                    IN const IvhSensorData* const pAccelData);

void RefineMagOffsetsWithGyro(IN PMAGN_DYNAMICCALI_T cali,
                      IN const ROTATION_VECTOR_T* calibratedMag,
                      IN const ROTATION_VECTOR_T* gyro,
                      IN const float magConfidence);
void RefineMagOffsets(IN PMAGN_DYNAMICCALI_T cali,
                      IN const ROTATION_VECTOR_T* calibratedMag,
                      IN const float magConfidence);

void MagDetectAnomalyWithGyro(IN PMAGN_DYNAMICCALI_T cali,
                      IN const ROTATION_VECTOR_T* const calibratedMag,
                      IN const ROTATION_VECTOR_T* const gyro,
                      OUT float* const magConfidence);
void MagDetectAnomaly(IN PMAGN_DYNAMICCALI_T cali,
                      IN const ROTATION_VECTOR_T* const calibratedMag,
                      OUT float* const magConfidence);
void InitializeMagDynamic(INOUT PMAGN_DYNAMICCALI_T cali, IN pIvhPlatform platform);
void UnInitializeMagDynamic(IN PMAGN_DYNAMICCALI_T cali);

#endif //_MAG_DYNAMIC_CALI_H
