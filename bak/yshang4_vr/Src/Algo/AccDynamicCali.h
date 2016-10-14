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

#if !defined(_ACC_DYNAMIC_CALI_H_)
#define _ACC_DYNAMIC_CALI_H

#include "SensorManagerTypes.h"
#include "Rotation.h"
#include "Matrix.h"
#include "platformCalibration.h"

#ifndef __GOD_BLESS_ACC__ //God bless me!
#define __GOD_BLESS_ACC__
typedef struct
{
    MATRIX_STRUCT_T a;
    MATRIX_STRUCT_T b;
} ACC_CALIBRATION_3D_PARAMS_STRUCT_T;

typedef enum
{
    ACC_CALIBRATION_STATE_INIT,
    ACC_CALIBRATION_STATE_RUNNING
} ACC_CALIBRATION_STATE_T;

typedef struct _ACCL_DYNAMICCALI_T
{
    ACC_CALIBRATION_STATE_T calibrationState; 

    ACC_CALIBRATION_3D_PARAMS_STRUCT_T magParams3D;

    // Current anomaly state
    BOOLEAN inAnomaly;

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
}ACCL_DYNAMICCALI_T, *PACCL_DYNAMICCALI_T;
#endif

void AccDynamicCali(IN PACCL_DYNAMICCALI_T cali, INOUT IvhSensorData* pSensorData);
void RefineAccOffsets(IN PACCL_DYNAMICCALI_T cali, IN const IvhSensorData* pSensorData);

void InitializeAccDynamic(INOUT PACCL_DYNAMICCALI_T cali, IN pIvhPlatform platform);
void UnInitializeAccDynamic(IN PACCL_DYNAMICCALI_T cali);

#endif //_ACC_DYNAMIC_CALI_H
