////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2013, Intel Corporation.  All Rights Reserved.       //
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

#include "DeviceOrientation6Axis.h"
#include "TMR.h"
#include "Rotation.h"
#include "stdlib.h"
#include "platformCalibration.h"
#include "CalibrationCommon.h"
#include "Matrix.h"
#include "Rotation.h"
#include "MotionCompensation.h"
#include "MagDynamicCali.h"
#include "BackupRegisters.h"

#include "AccelerometerCalibrated.h"
#include "GyroCalibrated.h"
#include "CompassCalibrated.h"
#include "CompassOrientation.h"

#include "Trace.h"

//--------------------------------------------------------------------------------------------------------------------------------
// DATA
//--------------------------------------------------------------------------------------------------------------------------------
#define MAX_HEADING_ERROR 179
#define MIN_HEADING_ERROR 0

#define DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX                        0.8f
#define DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_NOR                        0.6f
#define DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN                        0.1f

#define IVH_SENSOR_ORIENTATIONT_6_AXIS_POOL_TAG 'O6VI'

//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);

static ERROR_T CalculateRotation(pIvhSensorOrientation fusion);
static void ApplyDynamicLowPassFilter(pIvhSensorOrientation fusion, ROTATION_VECTOR_T* const mag, const ROTATION_VECTOR_T* const gyro);
//static BOOLEAN ShouldApplyZRT(pIvhSensorOrientation fusion);
static ERROR_T UpdateData(IN pIvhSensorOrientation fusion, INOUT IvhSensorData* const pSensorData);
static void EstimateHeadingError(IN const float magConfidence,
                                 IN const ROTATION_VECTOR_T incrementalRotationDegrees[NUM_AXES],
                                 OUT uint8_t* const pEstimatedHeadingError);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

pIvhSensor CreateSensorOrientation6Axis(const pIvhPlatform platform, pIvhSensor accl, pIvhSensor magn)
{
    pIvhSensorOrientation sensor = (pIvhSensorOrientation)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorOrientation), IVH_SENSOR_ORIENTATIONT_6_AXIS_POOL_TAG);

    if (sensor) 
    {
        // ZERO it!
        SAFE_FILL_MEM (sensor, sizeof (IvhSensorOrientation), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for orientation6axis");
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
    sensor->sensor.type = IVH_SENSOR_TYPE_ORIENTATION_6AXIS;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;
    accl->Attach(accl, &(sensor->sensor));
    magn->Attach(magn, &(sensor->sensor));

    ROTATION_MATRIX_T rm[] = {1, 0, 0, \
        0, 1, 0, \
        0, 0, 1};
    SAFE_MEMSET(sensor->lastCalibratedAccel, 0);
    SAFE_MEMSET(sensor->lastCalibratedGyro, 0);
    SAFE_MEMSET(sensor->lastCalibratedMag, 0);
    SAFE_MEMSET(sensor->lastRotatedMag, 0);
    SAFE_MEMSET(sensor->lastRawMag, 0);

    SAFE_MEMSET(sensor->lastMagCovariance, 0);

    sensor->magConfidence = 0;
    sensor->lastTimestampAccel = 0;
    sensor->lastTimestampGyro = 0;
    sensor->lastTimestampMag = 0;

    sensor->estimatedHeadingError = MAX_HEADING_ERROR;


    // Don't apply ZRT until at least one sample has been calculated and
    // SensorManager has been notified.
    sensor->atLeastOneSampleCalculated = FALSE;

    SAFE_MEMCPY(sensor->rotationMatrixStruct, rm);
    SAFE_MEMSET(sensor->driftCorrection, 0);

    SAFE_MEMSET(sensor->s_prevGyro, 0);
    sensor->s_motionlessTime = 0;
    sensor->s_startMotionlessTime = 0;

    sensor->s_firstTime = TRUE;

    SAFE_MEMSET(sensor->prevGyro, 0);
    SAFE_MEMSET(sensor->prevAccel, 0);
    sensor->previousTimestampMag = 0;

    sensor->platform = platform;
    InitRotationParameters(&sensor->Rotation);
    sensor->Calibration = platform->Calibrate;

    return (pIvhSensor)(sensor);
}

void DestroySensorOrientation6Axis(pIvhSensor me)
{
    pIvhSensorOrientation sensor = (pIvhSensorOrientation)(me);

    if(sensor != NULL)
    {
        SAFE_FREE_POOL(sensor, IVH_SENSOR_ORIENTATIONT_6_AXIS_POOL_TAG);
        sensor = NULL;
    }
}

//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------

static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type)
{
    ERROR_T retVal = ERROR_OK;
    pIvhSensorOrientation sensor = (pIvhSensorOrientation)(me);
    const int16_t MIN_ACCEL_CHANGE = 10;
    const float ACCEL_LPF_ALPHA = 0.7f;
    const float MAGN_LPF_ALPHA = 0.7f;

    switch(type)
    {
    case IVH_SENSOR_TYPE_ACCELEROMETER3D :
        // The normal minimum sensitivity for accel is 35mg.  We can set it
        // to zero and get all samples, but it's too jittery.  This is a secondary
        // filter that works as if we specified a sensitivity of 10mg
        if (fabs(sensor->lastCalibratedAccel[AXIS_X] - data->data.accel.xyz.x) > MIN_ACCEL_CHANGE  ||
            fabs(sensor->lastCalibratedAccel[AXIS_Y] - data->data.accel.xyz.y) > MIN_ACCEL_CHANGE ||
            fabs(sensor->lastCalibratedAccel[AXIS_Z] - data->data.accel.xyz.z) > MIN_ACCEL_CHANGE)
        {
            sensor->lastCalibratedAccel[AXIS_X] = (ROTATION_VECTOR_T)(data->data.accel.xyz.x);
            sensor->lastCalibratedAccel[AXIS_Y] = (ROTATION_VECTOR_T)(data->data.accel.xyz.y);
            sensor->lastCalibratedAccel[AXIS_Z] = (ROTATION_VECTOR_T)(data->data.accel.xyz.z);
        }

        // ZRT was removed from AccelerometerCalibrated, so there will be jitter in the incoming
        // accel.  Apply a weak LPF to reduce the jittery behavior
        ApplyLowPassFilter(sensor->lastCalibratedAccel, sensor->prevAccel, 3,ACCEL_LPF_ALPHA);

        sensor->lastTimestampAccel = data->timeStampInMs;
        break;
    case IVH_SENSOR_TYPE_MAGNETOMETER3D :
        sensor->lastCalibratedMag[AXIS_X] = data->data.mag.xyzCalibrated.x;
        sensor->lastCalibratedMag[AXIS_Y] = data->data.mag.xyzCalibrated.y;
        sensor->lastCalibratedMag[AXIS_Z] = data->data.mag.xyzCalibrated.z;
        sensor->lastRotatedMag[AXIS_X] = data->data.mag.xyzRotated.x;
        sensor->lastRotatedMag[AXIS_Y] = data->data.mag.xyzRotated.y;
        sensor->lastRotatedMag[AXIS_Z] = data->data.mag.xyzRotated.z;
        sensor->lastRawMag[AXIS_X] = (ROTATION_VECTOR_T)(data->data.mag.xyzRaw.x);
        sensor->lastRawMag[AXIS_Y] = (ROTATION_VECTOR_T)(data->data.mag.xyzRaw.y);
        sensor->lastRawMag[AXIS_Z] = (ROTATION_VECTOR_T)(data->data.mag.xyzRaw.z);
        memcpy(&sensor->lastMagCovariance, data->data.mag.covariance, (COVARIANCE_MATRIX_SIZE * sizeof(float)));

        if (sensor->platform->CalibrationFeatures.enableMagCalibratedDynamicFilter)
        {
            //use dynamic filter aligning with gyro speed to filter out jitter
            ApplyDynamicLowPassFilter(sensor, sensor->lastRotatedMag, sensor->lastCalibratedAccel);
        }
        else
        {
            ApplyLowPassFilter(sensor->lastRotatedMag, sensor->s_prevMag, 3, MAGN_LPF_ALPHA);
        }

        sensor->lastTimestampMag = data->timeStampInMs;
        Calibrate(me);
        break;
    default:
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]invalid sensor type, expect [IVH_SENSOR_TYPE_ACCELEROMETER3D|IVH_SENSOR_TYPE_GYROSCOPE3D|IVH_SENSOR_TYPE_MAGNETOMETER3D]");
    }

    return retVal;
}

static ERROR_T Calibrate(IvhSensor* me)
{
    ERROR_T retVal = ERROR_OK;
    IvhSensorData temp;
    pIvhSensorOrientation sensor = (pIvhSensorOrientation)(me);

    SAFE_MEMCPY(&temp, &sensor->input);

    CalculateRotation(sensor);

    UpdateData(sensor, &temp);

    SAFE_MEMCPY(&sensor->output, &temp);

    me->Notify(me);
    return retVal;
}

static ERROR_T CalculateRotation(pIvhSensorOrientation fusion)
{
    ERROR_T retVal = ERROR_OK;
    ROTATION_VECTOR_T incrementalRotationDegrees[3] = {0, 0, 0};

    //use mag to estimate motion, and compensate to acc...
    //ROTATION_VECTOR_T me[] = {fusion->lastCalibratedMag[0] - fusion->s_prevMag[0], fusion->lastCalibratedMag[1] - fusion->s_prevMag[1], fusion->lastCalibratedMag[2] - fusion->s_prevMag[2]};
    // Use the ROTATED mag with differential correction for finding direction
    TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "a(%f,%f,%f)m(%f,%f,%f)",
        fusion->lastCalibratedAccel[0],fusion->lastCalibratedAccel[1],fusion->lastCalibratedAccel[2],
        fusion->lastRotatedMag[0],fusion->lastRotatedMag[1],fusion->lastRotatedMag[2]);

    //check acc...
    if(fusion->lastCalibratedAccel[0]*fusion->lastCalibratedAccel[0]+fusion->lastCalibratedAccel[1]*fusion->lastCalibratedAccel[1]+fusion->lastCalibratedAccel[2]*fusion->lastCalibratedAccel[2]<10000)
    {
        return ERROR_OK;
    }
    MotionCompensate(fusion->lastCalibratedAccel, fusion->lastRotatedMag, fusion->rotationMatrixStruct);
    CalculateDCM(fusion->lastCalibratedAccel, fusion->lastRotatedMag, fusion->rotationMatrixStruct);

    if (fusion->lastTimestampMag > fusion->previousTimestampMag)
    {
        // Use the calibrated mag, NOT the rotated mag to check anomaly and refine offsets
        MagDetectAnomaly(fusion->Calibration, fusion->lastCalibratedMag,  &fusion->magConfidence);
        RefineMagOffsets(fusion->Calibration, fusion->lastRawMag, fusion->magConfidence);
        fusion->previousTimestampMag = fusion->lastTimestampMag;
    }

    EstimateHeadingError(fusion->magConfidence, incrementalRotationDegrees, &fusion->estimatedHeadingError);
    fusion->estimatedHeadingError = (uint8_t)(fusion->Calibration->Error);

    fusion->atLeastOneSampleCalculated = TRUE;

    return retVal;
}

static ERROR_T UpdateData(IN pIvhSensorOrientation fusion, INOUT IvhSensorData* const pSensorData)
{
    ERROR_T retVal = ERROR_OK;

    ROTATION_QUATERNION_T quaternionStruct[] = {0, 0, 0, 0};
    ROTATION_DATA_T yawYAxis, pitch, rollYAxis;

    ROTATION_MATRIX_T *outputRotationMatrix = fusion->rotationMatrixStruct;

    ConvertRotationToQuaternion(outputRotationMatrix, quaternionStruct);
    ConvertRotationToEulerAngles(outputRotationMatrix, &yawYAxis, &pitch, &rollYAxis);

    // Find yaw relative to Z axis by rotating the whole system around the X axis
    // and recomputing the Euler angles
    ROTATION_MATRIX_T rotate90DegreesX[] = { 1,   0,   0,
        0,   0,  -1,
        0,   1,   0 };

    ROTATION_MATRIX_T tmpRotation[9];
    RotationMatrixMultiply(tmpRotation,
        outputRotationMatrix,
        rotate90DegreesX);

    ROTATION_DATA_T yawZAxis, tmpPitch, rollZAxis;
    ConvertRotationToEulerAngles(tmpRotation, &yawZAxis, &tmpPitch, &rollZAxis);

    pSensorData->data.orientation.quaternion.x = quaternionStruct[QX];
    pSensorData->data.orientation.quaternion.y = quaternionStruct[QY];
    pSensorData->data.orientation.quaternion.z = quaternionStruct[QZ];
    pSensorData->data.orientation.quaternion.w = quaternionStruct[QW];

    pSensorData->data.orientation.rotationMatrix.m11 = fusion->rotationMatrixStruct[M11];
    pSensorData->data.orientation.rotationMatrix.m12 = fusion->rotationMatrixStruct[M12];
    pSensorData->data.orientation.rotationMatrix.m13 = fusion->rotationMatrixStruct[M13];

    pSensorData->data.orientation.rotationMatrix.m21 = fusion->rotationMatrixStruct[M21];
    pSensorData->data.orientation.rotationMatrix.m22 = fusion->rotationMatrixStruct[M22];
    pSensorData->data.orientation.rotationMatrix.m23 = fusion->rotationMatrixStruct[M23];

    pSensorData->data.orientation.rotationMatrix.m31 = fusion->rotationMatrixStruct[M31];
    pSensorData->data.orientation.rotationMatrix.m32 = fusion->rotationMatrixStruct[M32];
    pSensorData->data.orientation.rotationMatrix.m33 = fusion->rotationMatrixStruct[M33];

    pSensorData->data.orientation.pitch = pitch;
    pSensorData->data.orientation.yawYAxis = yawYAxis;
    pSensorData->data.orientation.yawZAxis = yawZAxis;
    pSensorData->data.orientation.rollZAxis = rollZAxis;
    pSensorData->data.orientation.roll = rollYAxis;

    pSensorData->data.orientation.estimatedHeadingError = fusion->estimatedHeadingError;

    pSensorData->data.orientation.magRaw.x = (int32_t)(fusion->lastRotatedMag[AXIS_X]);
    pSensorData->data.orientation.magRaw.y = (int32_t)(fusion->lastRotatedMag[AXIS_Y]);
    pSensorData->data.orientation.magRaw.z = (int32_t)(fusion->lastRotatedMag[AXIS_Z]);

    //pSensorData->timeStampInMs = TmrGetMillisecondCounter();
    pSensorData->timeStampInMs = fusion->lastTimestampMag; //use mag ts

    return retVal;
}

// Estimate heading error.  Error may range from 0 (highest accuracy) to 179
// indicating number of degrees of estimated error.
// The error is set to zero when magConfidence is > 0.75.  Accuracy degrades
// and estimated heading error increases as we accumulate degrees of motion
// using only the gyro as a heading reference.
static void EstimateHeadingError(IN const float magConfidence,
                                 IN const ROTATION_VECTOR_T incrementalRotationDegrees[NUM_AXES],
                                 OUT uint8_t* const pEstimatedHeadingError)
{
    // If mag confidence is high, reset the accumulators
    const float MAG_CONFIDENCE_HIGH = 0.75;
    if (magConfidence >= MAG_CONFIDENCE_HIGH)
    {
        BackupRegisterWriteFloat(BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_X_A,
            BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_X_B,
            0);
        BackupRegisterWriteFloat(BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Y_A,
            BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Y_B,
            0);
        BackupRegisterWriteFloat(BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Z_A,
            BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Z_B,
            0);
        *pEstimatedHeadingError = 0;
    }
    else
    {
        // Always read the accumulated degree change from the backup registers.
        // This ensures that the state will be preserved across a reboot
        ROTATION_VECTOR_T accumulatedDegreeChange[3];
        BackupRegisterReadFloat(BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_X_A,
            BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_X_B,
            &(accumulatedDegreeChange[AXIS_X]));
        BackupRegisterReadFloat(BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Y_A,
            BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Y_B,
            &(accumulatedDegreeChange[AXIS_Y]));
        BackupRegisterReadFloat(BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Z_A,
            BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Z_B,
            &(accumulatedDegreeChange[AXIS_Z]));
        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "accumulatedDegreeChange(%f,%f,%f),",accumulatedDegreeChange[AXIS_X],accumulatedDegreeChange[AXIS_Y],accumulatedDegreeChange[AXIS_Z]);
        //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "incrementalRotationDegrees(%f,%f,%f),",incrementalRotationDegrees[AXIS_X],incrementalRotationDegrees[AXIS_Y],incrementalRotationDegrees[AXIS_Z]);

        accumulatedDegreeChange[AXIS_X] += incrementalRotationDegrees[AXIS_X];
        accumulatedDegreeChange[AXIS_Y] += incrementalRotationDegrees[AXIS_Y];
        accumulatedDegreeChange[AXIS_Z] += incrementalRotationDegrees[AXIS_Z];

        BackupRegisterWriteFloat(BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_X_A,
            BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_X_B,
            accumulatedDegreeChange[AXIS_X]);
        BackupRegisterWriteFloat(BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Y_A,
            BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Y_B,
            accumulatedDegreeChange[AXIS_Y]);
        BackupRegisterWriteFloat(BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Z_A,
            BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Z_B,
            accumulatedDegreeChange[AXIS_Z]);

        // Use the vector magnitude of the accumulated rotation as a scalar measure
        // of how much rotation has happened since we last used the mag as a
        // heading reference
        ROTATION_DATA_T vectorMagnitudeAccumulatedRotation =
            sqrt(accumulatedDegreeChange[AXIS_X] * accumulatedDegreeChange[AXIS_X] +
            accumulatedDegreeChange[AXIS_Y] * accumulatedDegreeChange[AXIS_Y] +
            accumulatedDegreeChange[AXIS_Z] * accumulatedDegreeChange[AXIS_Z]);

        // Through experimentation we see that gyro variation is about 5 percent
        // As degree changes build up without a mag reference, the estimated heading error is
        // 5 percent of the vector magnitude of the accumulated degree change
        const float GYRO_ERROR_PERCENT = 0.05f;  //  5 percent
        *pEstimatedHeadingError = MIN(MAX_HEADING_ERROR, (uint8_t)(fabsf(vectorMagnitudeAccumulatedRotation * GYRO_ERROR_PERCENT)));
    }
}

static void ApplyDynamicLowPassFilter(pIvhSensorOrientation fusion, ROTATION_VECTOR_T* const mag, const ROTATION_VECTOR_T* const acc)
{
//    static uint32_t last_state = 0;
    uint32_t state = 0;
//...cjy    static ROTATION_VECTOR_T an[]={acc[0],acc[1],acc[2]}; //an: last time 's accel
    static ROTATION_VECTOR_T an[]={.0f,.0f,-1.f}; //an: last time 's accel
    ROTATION_DATA_T ann = sqrtf(acc[0]*acc[0]+acc[1]*acc[1]+acc[2]*acc[2]); //norm of an
    ROTATION_VECTOR_T anp[]={acc[0],acc[1],acc[2]}; //anp: this time 's accel
    ROTATION_DATA_T anpn = sqrtf(anp[0]*anp[0]+anp[1]*anp[1]+anp[2]*anp[2]); //norm of anp
    ROTATION_VECTOR_T da[] = {anp[0]-an[0],anp[1]-an[1],anp[2]-an[2]};

    //calcuate movement based on ann and anpn
    ROTATION_DATA_T dann = fabs(ann - 1000.0f); //delta
    ROTATION_DATA_T danpn = fabs(anpn - 1000.0f); //delta
    ROTATION_DATA_T dannpn = fabs(ann - anpn); //delta
    ROTATION_DATA_T dan = sqrtf(da[0]*da[0]+da[1]*da[1]+da[2]*da[2]);

    ROTATION_DATA_T alpha[NUM_AXES]={1.0f,1.0f,1.0f};
    static ROTATION_DATA_T last_alpha[NUM_AXES]={1.0f,1.0f,1.0f};
    //min alpha for 3 axis
    const float minAlpha[NUM_AXES] = {DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN,
        DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN,
        DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN};
    //max alpha for 3 axis
    const float maxAlpha[NUM_AXES] = {DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX,
        DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX,
        DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX};	

    if(dann>500 ||danpn >500 || dan >500)
    {
        //fast movement, use DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX
        alpha[AXIS_X] = maxAlpha[AXIS_X];
        alpha[AXIS_Y] = maxAlpha[AXIS_Y];
        alpha[AXIS_Z] = maxAlpha[AXIS_Z];
        state = 2;
    }
    else if(dann < 100 && danpn < 100 && dan <100)
    {
        if(dannpn < 20 && dan <20)
        {
            //still, use DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN
            alpha[AXIS_X] = minAlpha[AXIS_X];
            alpha[AXIS_Y] = minAlpha[AXIS_Y];
            alpha[AXIS_Z] = minAlpha[AXIS_Z];
            state = 0;
        }
        else
        {
            //slow movement, set DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN to DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_NOR
            ROTATION_DATA_T d = MAX(MAX(dann,danpn),dan);
            ROTATION_DATA_T a = (DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_NOR - DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN)*d/100 + DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN;
            alpha[AXIS_X] = a;
            alpha[AXIS_Y] = a;
            alpha[AXIS_Z] = a;
            state = 1;
        }
    }
    else
    {
        //normal movement, set DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_NOR to DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX
        ROTATION_DATA_T d = MAX(MAX(dann,danpn),dan) - 100;
        ROTATION_DATA_T a = (DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX - DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_NOR)*d/400 + DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_NOR;
        alpha[AXIS_X] = a;
        alpha[AXIS_Y] = a;
        alpha[AXIS_Z] = a;
        state = 3;
    }

    if (TRUE == fusion->s_firstTime)
    {
        fusion->s_prevMag[AXIS_X] = mag[AXIS_X];
        fusion->s_prevMag[AXIS_Y] = mag[AXIS_Y];
        fusion->s_prevMag[AXIS_Z] = mag[AXIS_Z];
        fusion->s_firstTime = FALSE;
    }
    else 
    {
        /* we can't change alpha sharply, or virtual gyro will change sharply accordingly
        if(last_state != state)
        {
            alpha[AXIS_X] = DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX;
            alpha[AXIS_Y] = DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX;
            alpha[AXIS_Z] = DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX;
            last_state = state;
        }
        */
        for(int i=AXIS_X; i<NUM_AXES; ++i)
        {
            // Apply the LPF separately on each axis. alpha for every axis is calculated above.
            ApplyLowPassFilter(&(mag[i]), &(fusion->s_prevMag[i]), 1, alpha[i]);
        }
    }
    last_alpha[AXIS_X]=alpha[AXIS_X];
    last_alpha[AXIS_Y]=alpha[AXIS_Y];
    last_alpha[AXIS_Z]=alpha[AXIS_Z];
    an[0]=acc[0];
    an[1]=acc[1];
    an[2]=acc[2];
    //TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "a(%f,%f,%f)m(%f,%f,%f)alpha(%f,%f,%f),",acc[0],acc[1],acc[2],dann,danpn,dannpn,alpha[AXIS_X],alpha[AXIS_Y],alpha[AXIS_Z]);
}
