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

#include "DeviceOrientation9Axis.h"
#include "TMR.h"
#include "Rotation.h"
#include "stdlib.h"
#include "platformCalibration.h"
#include "CalibrationCommon.h"
#include "Matrix.h"
#include "Rotation.h"
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

#define DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX                        0.5f
#define DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN                        0.05f

#define IVH_SENSOR_ORIENTATIONT_9AXIS__POOL_TAG 'O9VI'
//------------------------------------------------------------------------------
// FUNCTIONS definition
//------------------------------------------------------------------------------
static ERROR_T DataUpdate(IvhSensor* me, const IvhSensorData* data, const IvhSensorType type);
static ERROR_T Calibrate(IvhSensor* me);

static ERROR_T CalculateRotation(pIvhSensorOrientation fusion);
static void ApplyDynamicLowPassFilter(pIvhSensorOrientation fusion, ROTATION_VECTOR_T* const mag, const ROTATION_VECTOR_T* const gyro);
static BOOLEAN ShouldApplyZRT(pIvhSensorOrientation fusion);
static ERROR_T UpdateData(IN pIvhSensorOrientation fusion, INOUT IvhSensorData* const pSensorData);
static void EstimateHeadingError(IN const float magConfidence,
                                 IN const ROTATION_VECTOR_T incrementalRotationDegrees[NUM_AXES],
                                 OUT uint8_t* const pEstimatedHeadingError);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

pIvhSensor CreateSensorOrientation9Axis(const pIvhPlatform platform, pIvhSensor accl, pIvhSensor gyro, pIvhSensor magn)
{
    pIvhSensorOrientation sensor = (pIvhSensorOrientation)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhSensorOrientation), IVH_SENSOR_ORIENTATIONT_9AXIS__POOL_TAG);

    if (sensor) 
    {
        // ZERO it!
        SAFE_FILL_MEM (sensor, sizeof (IvhSensorOrientation), 0);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for orientation9axis");
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
    sensor->sensor.type = IVH_SENSOR_TYPE_ORIENTATION_9AXIS;
    sensor->sensor.d[0] = &sensor->input;
    sensor->sensor.d[1] = &sensor->output;
    accl->Attach(accl, &(sensor->sensor));
    gyro->Attach(gyro, &(sensor->sensor));
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

void DestroySensorOrientation9Axis(pIvhSensor me)
{
    pIvhSensorOrientation sensor = (pIvhSensorOrientation)(me);

    if(sensor != NULL)
    {
        SAFE_FREE_POOL(sensor, IVH_SENSOR_ORIENTATIONT_9AXIS__POOL_TAG);
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

        sensor->lastTimestampAccel = data->timeStampInMs;
        break;
    case IVH_SENSOR_TYPE_GYROSCOPE3D :
        sensor->lastCalibratedGyro[AXIS_X] = (ROTATION_VECTOR_T)(data->data.gyro.xyz.x);
        sensor->lastCalibratedGyro[AXIS_Y] = (ROTATION_VECTOR_T)(data->data.gyro.xyz.y);
        sensor->lastCalibratedGyro[AXIS_Z] = (ROTATION_VECTOR_T)(data->data.gyro.xyz.z);
        sensor->lastTimestampGyro = data->timeStampInMs;
        Calibrate(me);
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
        memcpy(&sensor->lastMagCovariance,
            data->data.mag.covariance,
            (COVARIANCE_MATRIX_SIZE * sizeof(float)));
        sensor->lastTimestampMag = data->timeStampInMs;
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

ERROR_T CalculateRotation(pIvhSensorOrientation fusion)
{
    ERROR_T retVal = ERROR_OK;

    // ZRT was removed from GyroCalibrated, so there will be jitter in the incoming
    // gyro.  Apply a weak LPF to reduce the jittery behavior
    const float GYRO_LPF_ALPHA = 0.4f;

    ApplyLowPassFilter(fusion->lastCalibratedGyro,
        fusion->prevGyro,
        3,
        GYRO_LPF_ALPHA);

    // ZRT was removed from AccelerometerCalibrated, so there will be jitter in the incoming
    // accel.  Apply a weak LPF to reduce the jittery behavior
    const float ACCEL_LPF_ALPHA = 0.3f;

    ApplyLowPassFilter(fusion->lastCalibratedAccel ,
        fusion->prevAccel,
        3,
        ACCEL_LPF_ALPHA);

    // Calibrated magnetometer microdriver just maps the axes for this device
    // We need to send it to MagDynamicCali to get the mag data centered at zero
    // and mapped to a unit circle

    if (fusion->platform->CalibrationFeatures.enableMagCalibratedDynamicFilter)
    {
        //use dynamic filter aligning with gyro speed to filter out jitter
        ApplyDynamicLowPassFilter(fusion, fusion->lastCalibratedMag, fusion->lastCalibratedGyro);
    }

    // If gyro has been motionless for some time, there is no need to
    // run the device orientation algo.  This will prevent jitter at rest.
    if (TRUE == fusion->atLeastOneSampleCalculated && TRUE == ShouldApplyZRT(fusion))
    {
        fusion->lastCalibratedGyro[AXIS_X] = fusion->lastCalibratedGyro[AXIS_Y] = fusion->lastCalibratedGyro[AXIS_Z] = 0;
    }
    else
    {

        ROTATION_VECTOR_T incrementalRotationDegrees[3] = {0, 0, 0};

        // Use the ROTATED mag with differential correction for finding direction
        TraceLog(TRACE_LEVEL_VERBOSE, TRACE_ALGO, "a(%f,%f,%f)g(%f,%f,%f)m(%f,%f,%f)",
            fusion->lastCalibratedAccel[0],fusion->lastCalibratedAccel[1],fusion->lastCalibratedAccel[2],
            fusion->lastCalibratedGyro[0],fusion->lastCalibratedGyro[1],fusion->lastCalibratedGyro[2],
            fusion->lastRotatedMag[0],fusion->lastRotatedMag[1],fusion->lastRotatedMag[2]);

        CalculateRotationUsingGyro(&fusion->Rotation,
            fusion->lastCalibratedAccel , fusion->lastCalibratedGyro , fusion->lastRotatedMag,
            fusion->lastTimestampGyro,
            fusion->lastTimestampMag,
            fusion->lastTimestampAccel,
            fusion->magConfidence,
            fusion->rotationMatrixStruct,
            fusion->driftCorrection,
            incrementalRotationDegrees);

        if (fusion->lastTimestampMag > fusion->previousTimestampMag)
        {
            // Use the calibrated mag, NOT the rotated mag to check anomaly and refine offsets
            MagDetectAnomalyWithGyro(fusion->Calibration, fusion->lastCalibratedMag, fusion->lastCalibratedGyro , &fusion->magConfidence);
            RefineMagOffsetsWithGyro(fusion->Calibration, fusion->lastRawMag, fusion->lastCalibratedGyro, fusion->magConfidence);
            fusion->previousTimestampMag = fusion->lastTimestampMag;
        }

        EstimateHeadingError(fusion->magConfidence, incrementalRotationDegrees, &fusion->estimatedHeadingError);
        fusion->estimatedHeadingError = (uint8_t)(fusion->Calibration->Error);
    }

    fusion->atLeastOneSampleCalculated = TRUE;

    return retVal;
}

static void ApplyDynamicLowPassFilter(pIvhSensorOrientation fusion, ROTATION_VECTOR_T* const mag, const ROTATION_VECTOR_T* const gyro)
{

    // Calculate an alpha setting based on gyro movement
    float gyroNorm = sqrtf(gyro[AXIS_X] * gyro[AXIS_X] +
        gyro[AXIS_Y] * gyro[AXIS_Y] +
        gyro[AXIS_Z] * gyro[AXIS_Z]);
    float gyroUnitVector[NUM_AXES] = {0, 0, 0};

    if (TRUE == fusion->s_firstTime)
    {
        fusion->s_prevMag[AXIS_X] = mag[AXIS_X];
        fusion->s_prevMag[AXIS_Y] = mag[AXIS_Y];
        fusion->s_prevMag[AXIS_Z] = mag[AXIS_Z];
        fusion->s_firstTime = FALSE;
    }
    else 
    {	
        //min alpha for 3 axis
        const float minAlpha[NUM_AXES] = {DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN,
            DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN,
            DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MIN};
        //max alpha for 3 axis
        const float maxAlpha[NUM_AXES] = {DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX,
            DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX,
            DEFAULT_MAG_DYNAMIC_FILTER_ALPHA_MAX};	

        //alpha when system is still
        const float ALPHA_AT_STILL = 0.0001f;			
        const float GYRO_MOVING_SLOW_SPEED = 2000.0f;
        const float GYRO_MOVING_FAST_SPEED = 60000.0f;
        float lpfAlpha[NUM_AXES];
        float gyroPercentMax;
        uint8_t i;

        for(i=AXIS_X; i<NUM_AXES; ++i)
        {
            if(0 != gyroNorm) 
            {
                //get the proportion of each axis gyro speed to gyro norm, lpf is distributed aligning to (1 - gyroUnitVector[i]).
                gyroUnitVector[i] = fabsf(gyro[i]) / gyroNorm;
            }
            //if in very slow speed rotating, the alpha should decrase from minAlpha towards 0, but the minima is ALPHA_AT_STILL.
            //the decrase will go down linear by gyroPercentMax and distributed to 3 axis by (1 - gyroUnitVector[i]).
            if (gyroNorm < GYRO_MOVING_SLOW_SPEED)
            {

                gyroPercentMax = (gyroNorm / GYRO_MOVING_SLOW_SPEED);
                lpfAlpha[i] = MAX(minAlpha[i] * gyroPercentMax * (1 - gyroUnitVector[i]),ALPHA_AT_STILL);
            }
            else

                //if in very fast speed rotating, the alpha should remain the maxim alpha.
                //alhpa will be distributed to 3 axis by (1 - gyroUnitVector[i]).
                if (gyroNorm > GYRO_MOVING_FAST_SPEED)
                {
                    lpfAlpha[i] = maxAlpha[i] * (1 - gyroUnitVector[i]);
                }

                //if in normal speed rotating, the alpha should change from minAlpha towards maxAlpha.
                //the alpha change will be linear according to gyroPercentMax and distributed to 3 axis by (1 - gyroUnitVector[i]).
                else
                {
                    gyroPercentMax =
                        ((gyroNorm - GYRO_MOVING_SLOW_SPEED) / (GYRO_MOVING_FAST_SPEED - GYRO_MOVING_SLOW_SPEED));
                    lpfAlpha[i] = (minAlpha[i] + (maxAlpha[i] - minAlpha[i]) * gyroPercentMax) * (1 - gyroUnitVector[i]);
                }

                // Apply the LPF separately on each axis. alpha for every axis is calculated above.
                ApplyLowPassFilter(&(mag[i]), &(fusion->s_prevMag[i]), 1, lpfAlpha[i]);
        }
    }
}

// Test if gyro is stationary.  The zero rate threshold in
// GyroCalibrated will set an axis to zero if below the ZRT.
// If all three axes stay below zero for ZRT_MOTIONLESS_TIME
// then we consider the gyro to be motionless.  Any sample that
// exceeds ZRT resets the motionless counter.
static BOOLEAN ShouldApplyZRT(pIvhSensorOrientation fusion)
{

    // Apply LPF before checking motionless state
    const float alphaGyro = 0.05f;
    float avgGyro[3] = {fusion->lastCalibratedGyro[AXIS_X], 
        fusion->lastCalibratedGyro[AXIS_Y],
        fusion->lastCalibratedGyro[AXIS_Z]};
    ApplyLowPassFilter(avgGyro, fusion->s_prevGyro, 3, alphaGyro);

    // If we're not moving, then add up samples and average them to find
    // new gyro offsets.
    // Milli-dps values below this threshold will be considered as motionless
    const uint16_t GYRO_MOTION_THRESHOLD = 2000;  // 1.5 dps from the AVERAGE gyro norm
    const float MAG_COVARIANCE_THRESHOLD = 2000*1000.0f;

    const uint32_t ZRT_MOTIONLESS_TIME = 1000; // ms

    if (FALSE == CalibrationDeviceIsMotionless(fusion->lastCalibratedGyro,
        avgGyro, 
        fusion->lastMagCovariance,
        GYRO_MOTION_THRESHOLD,
        MAG_COVARIANCE_THRESHOLD))
    {
        fusion->s_startMotionlessTime = TmrGetMillisecondCounter()/100;
        fusion->s_motionlessTime = 0;
    }
    else
    {
        if (fusion->s_motionlessTime < ZRT_MOTIONLESS_TIME)
        {
            uint32_t timeNow = TmrGetMillisecondCounter()/100;

            if (timeNow > fusion->s_startMotionlessTime)
            {
                fusion->s_motionlessTime = timeNow - fusion->s_startMotionlessTime;
            }
            else  // Handle rollover
            {
                fusion->s_motionlessTime = (0xffffffffu - fusion->s_startMotionlessTime) + timeNow;
            }
        }
    }

    if (fusion->s_motionlessTime >= ZRT_MOTIONLESS_TIME)
    {
        return TRUE;
    }

    return FALSE;
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

    pSensorData->timeStampInMs = TmrGetMillisecondCounter();

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
