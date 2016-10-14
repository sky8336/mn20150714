////////////////////////////////////////////////////////////////////////////////
//      Copyright (c) 2012, Intel Corporation.  All Rights Reserved.          //
//                                                                            //
//           Portions derived with permission from STM32Lib                   //
//               Copyright (c) 2010, STMicroelectronics.                      //
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

#include <string.h>
#include "platformCalibration.h"
#include "math.h"

#include "Trace.h"
/********cjy 
// {86359A08-B9D3-4CF0-8006-17FCFFB509E8}
static const GUID IVSFHUBHIDMINI_VARIABLE_GUID = 
{ 0x86359a08, 0xb9d3, 0x4cf0, { 0x80, 0x6, 0x17, 0xfc, 0xff, 0xb5, 0x9, 0xe8 } };
******/
typedef struct _PLATFORM_CALIBRATION
{
    ULONG calibrated;
    ULONG spen;
    ULONG shake_th;
    ULONG shake_shock;
    ULONG shake_quiet;
    ULONG accl_tolarence;
    ULONG gyro_tolarence;
    ULONG magn_tolarence;
    ULONG stableMS;
    ULONG magnx;
    ULONG magny;
    ULONG magnz;
    ULONG magnxnx;
    ULONG magnxny;
    ULONG magnxnz;
    ULONG magnyux;
    ULONG magnyuy;
    ULONG magnyuz;
    ULONG magnxsx;
    ULONG magnxsy;
    ULONG magnxsz;
    ULONG magnydx;
    ULONG magnydy;
    ULONG magnydz;
    ULONG magnnx;
    ULONG magnny;
    ULONG magnnz;
    ULONG magnsx;
    ULONG magnsy;
    ULONG magnsz;
    ULONG acclx;
    ULONG accly;
    ULONG acclz;
    ULONG acclzx;//horizontal
    ULONG acclzy;
    ULONG acclzz;
    ULONG acclyx;//vertical
    ULONG acclyy;
    ULONG acclyz;
    ULONG gyrox;
    ULONG gyroy;
    ULONG gyroz;
    ULONG gyrozx;//horizontal
    ULONG gyrozy;
    ULONG gyrozz;
    ULONG gyroyx;//vertical
    ULONG gyroyy;
    ULONG gyroyz;
    ULONG alscurve[20];
    ULONG als_multiplier;
}*PPLATFORM_CALIBRATION;

#define IVH_FUSION_PLATFORM_POOL_TAG 'PHVI'
// X=2, Y=3, Z=1
#define POSITION_XYZ    2,3,1
#define POSITION_XZY    2,1,3
#define POSITION_YXZ    3,2,1
#define POSITION_YZX    3,1,2
#define POSITION_ZYX    1,3,2
#define POSITION_ZXY    1,2,3

#define REVERSE_NONE    0,0,0
#define REVERSE_X       1,0,0
#define REVERSE_XY      1,1,0
#define REVERSE_XZ      1,0,1
#define REVERSE_XYZ     1,1,1
#define REVERSE_Y       0,1,0
#define REVERSE_YZ      0,1,1
#define REVERSE_Z       0,0,1
#define DEFAULT_OFFSET              { 0, 0, 0 }
#define DEFAULT_ELLIPSE             { 0, 0, 0 }
#define DEFAULT_ORIENTATION         { POSITION_XYZ, REVERSE_NONE }
#define DEFAULT_CORRECTION_MATRIX   \
{ 1.0, 0.0, 0.0,                \
    0.0, 1.0, 0.0,                \
    0.0, 0.0, 1.0,                \
    0.0, 0.0, 0.0 }
#define DEFAULT_ZERO_RATE_THRESHOLD 0
#define DEFAULT_ALS_MULTIPLIER      125
#define DEFAULT_ALR_CURVE           \
{ 35,  1,                       \
    42,  9,                       \
    53,  38,                      \
    71,  65,                      \
    76,  146,                     \
    85,  296,                     \
    93,  545,                     \
    98,  924,                     \
    100, 1254,                    \
    100, 1700}
#define DEFAULT_GYRO_BOOST          1.00f
#define DEFAULT_HEADING_CORRECTION     \
{ 0, 0, 0, 0 }

// define and fill in the default platform calibration that will be 
// updated by calibaration in the Sensor App flash section.
// The code below overwrites this with the values from the
// Calibration flash section if they are present.  This allows the best of 
// both worlds - compile-time calibration, usable when working in the debugger,
// and in-the-field (factory, etc.) calibration, done by OEMs.
const static PLATFORM_CALIBRATION_T DefaultPlatformCalibration= {
    /*.accel = */
    {
        /*    .orientation        = */DEFAULT_ORIENTATION,
        /*    .correctionMatrix   = */DEFAULT_CORRECTION_MATRIX,
        /*    .zeroRateThreshold  = */DEFAULT_ZERO_RATE_THRESHOLD,
    },
    /*.als   = */
    {
        /*    .multiplier         = */DEFAULT_ALS_MULTIPLIER,
    },
    /*.mag   = */
    {
        /*    .orientation        = */DEFAULT_ORIENTATION,
        /*    .offset             = */DEFAULT_OFFSET,
        /*    .ellipse            = */DEFAULT_ELLIPSE,
    },
    /*.gyro  = */
    {
        /*    .orientation        = */DEFAULT_ORIENTATION,
        /*    .offset             = */DEFAULT_OFFSET,
        /*    .zeroRateThreshold  = */DEFAULT_ZERO_RATE_THRESHOLD,
    },
    /*.alrCurve               = */DEFAULT_ALR_CURVE,
    /*.protractor = */
    {
        /*    .orientation        = */DEFAULT_ORIENTATION,
        /*    .correctionMatrix   = */DEFAULT_CORRECTION_MATRIX,
        /*    .zeroRateThreshold  = */DEFAULT_ZERO_RATE_THRESHOLD,
    },
    /*.mag3D   = */
    {
        /*    .correctionMatrix   = */DEFAULT_CORRECTION_MATRIX,
    },
    /*.gyroCorrectionMatrix  = */
    {
        /*    .correctionMatrix   = */DEFAULT_CORRECTION_MATRIX,
    },
    /*.headingCorrection = */
    {
        /*    .correctionZUp          = */DEFAULT_HEADING_CORRECTION,
        /*    .correctionZDn          = */DEFAULT_HEADING_CORRECTION,
        /*    .correctionXUp          = */DEFAULT_HEADING_CORRECTION,
        /*    .correctionXDn          = */DEFAULT_HEADING_CORRECTION,
        /*    .correctionYUp          = */DEFAULT_HEADING_CORRECTION,
        /*    .correctionYDn          = */DEFAULT_HEADING_CORRECTION,
    }
};

#define DEFAULT_ENABLE_MAG_LOW_PASS_FILTER                          TRUE
#define DEFAULT_ENABLE_MAG_DYNAMIC_FILTER                          	TRUE
#define DEFAULT_MAG_LOW_PASS_FILTER_ALPHA                           1.f
#define DEFAULT_ENABLE_ACC_LOW_PASS_FILTER                          TRUE
#define DEFAULT_ACC_LOW_PASS_FILTER_ALPHA                           1.f

// TODO(bwoodruf) 
// This number was derived experimentally on the development board.
// It needs to be exposed through the calibration DFU so that it
// may be tuned if needed for other platforms
#define DEFAULT_ANOMALY_TRIGGER_POINT                              (0.001f*330)

//  TODO(bwoodruf): In the future, these should be defined in the
//  platform calibration flash memory, but we don't have time to update all the tools
//  to read / write them right now.
const static PLATFORM_CALIBRATION_FEATURES_T  DefaultCalibrationFeatures =
{
    /*.enableMagnetometerLowPassFilter                = */DEFAULT_ENABLE_MAG_LOW_PASS_FILTER,
    /*.enableMagCalibratedDynamicFilter				= */DEFAULT_ENABLE_MAG_DYNAMIC_FILTER,
    /*.enableAccelerometerLowPassFilter                = */DEFAULT_ENABLE_ACC_LOW_PASS_FILTER,
};

#define DEFAULT_CALIBRATION_TYPE                              (0) /*not calibrated*/
const static PLATFORM_CALIBRATION_SETTINGS_T  DefaultCalibrationSettings =
{
    /*.anomalyTriggerPoint                            = */DEFAULT_ANOMALY_TRIGGER_POINT,
    /*.magLowPassFilterAlpha                          = */DEFAULT_MAG_LOW_PASS_FILTER_ALPHA,
    /*.gyroBoost                                      = */DEFAULT_GYRO_BOOST,
    /*.calibration                                    = */DEFAULT_CALIBRATION_TYPE,
    /*.accLowPassFilterAlpha                          = */DEFAULT_ACC_LOW_PASS_FILTER_ALPHA,
};

#define GYROMETER_ROTATE_THRESHOLD 45000
#define ACCELEROMETER_ROTATE_THRESHOLD 800

static ERROR_T InitlizeFusionParametersFromFirmware(PPLATFORM_CALIBRATION pPlatform);
static PPLATFORM_CALIBRATION CreateCalibrationData(void);
static void DestoryCalibrationData(PPLATFORM_CALIBRATION pPlatform);
static void PlatformCalibrationGetOrientation(PLATFORM_CALIBRATION* cali, CALIBRATION_ORIENTATION_STRUCT_T* a, CALIBRATION_ORIENTATION_STRUCT_T* g, CALIBRATION_ORIENTATION_STRUCT_T* m);

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------

/* Called early in the boot sequence to update the compile time defaults 
* above with anything that might have changed in the field */
pIvhPlatform CreatePlatform(PULONG calibrate)
{
    TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "%!FUNC! Entry");

    pIvhPlatform pPlatform = (pIvhPlatform)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(IvhPlatform),IVH_FUSION_PLATFORM_POOL_TAG);
    if (pPlatform == NULL) 
    {
        //  No memory
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "not enough memory");
        return NULL;
    }

    SAFE_FILL_MEM(pPlatform, sizeof (IvhPlatform), 0);
    SAFE_MEMCPY(&pPlatform->CalibrationFeatures, &DefaultCalibrationFeatures);
    SAFE_MEMCPY(&pPlatform->CalibrationSettings, &DefaultCalibrationSettings);
    SAFE_MEMCPY(&pPlatform->PlatformCalibration, &DefaultPlatformCalibration);

    PPLATFORM_CALIBRATION cali = CreateCalibrationData();
    if(cali == NULL)
    {
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!] Can't initliaze platform data!");
        return NULL;
    }

    pPlatform->cali = cali;

    *calibrate = cali->calibrated;
    if(cali->calibrated >= 1)
    {
        pPlatform->CalibrationSettings.calibration = cali->calibrated; //remember calibration type(per system/per model)
        pPlatform->PlatformCalibration.als.multiplier = cali->als_multiplier;
        PlatformCalibrationGetOrientation(cali, &pPlatform->PlatformCalibration.accel.orientation, &pPlatform->PlatformCalibration.gyro.orientation, &pPlatform->PlatformCalibration.mag.orientation);
        INT32_VECTOR3_T data = {cali->acclzx, cali->acclzy, cali->acclzz};
        CalibrationSwapAxes(&data, &pPlatform->PlatformCalibration.accel.orientation);
        pPlatform->PlatformCalibration.protractor.correctionMatrix.m10 = -(float)(data.x); //accl x- offset
        pPlatform->PlatformCalibration.protractor.correctionMatrix.m20 = -(float)(data.y); //accl y- offset
        pPlatform->PlatformCalibration.protractor.correctionMatrix.m30 = -(float)(data.z) - 1000.0f; //accl z- offset

        INT32_VECTOR3_T gyro = {cali->gyrox, cali->gyroy, cali->gyroz};
        CalibrationSwapAxes(&gyro, &pPlatform->PlatformCalibration.gyro.orientation);
        pPlatform->PlatformCalibration.gyro.offset.offsetX = -(int16_t)(gyro.x);
        pPlatform->PlatformCalibration.gyro.offset.offsetY = -(int16_t)(gyro.y);
        pPlatform->PlatformCalibration.gyro.offset.offsetZ = -(int16_t)(gyro.z);

        INT32_VECTOR3_T magn = {(cali->magnxnx + cali->magnxsx)/2, (cali->magnxny + cali->magnxsy)/2, (cali->magnxnz + cali->magnxsz)/2};
        CalibrationSwapAxes(&magn, &pPlatform->PlatformCalibration.mag.orientation);
        pPlatform->PlatformCalibration.mag.offset.offsetX = (int16_t)(magn.x); //magn x-offset
        pPlatform->PlatformCalibration.mag.offset.offsetY = (int16_t)(magn.y); //magn y-offset
        pPlatform->PlatformCalibration.mag.offset.offsetZ = (int16_t)(magn.z); //magn z-offset
        //for mag 3d
#define DEGREE2RADIAN (3.1415926f/180)
        //#define ROTATEANGLE -37.58f
#define ROTATEANGLE .0f
        float cosangle = cosf(ROTATEANGLE*DEGREE2RADIAN);
        float sinangle = sinf(ROTATEANGLE*DEGREE2RADIAN);
        float rotate[9] = {1.0f, .0f, .0f,\
            .0f, cosangle, sinangle,\
            .0f, -sinangle, cosangle};

        //Platform.PlatformCalibration.mag3D.correctionMatrix.m10 = (float)(Platform.PlatformCalibration.mag.offset.offsetX);
        //Platform.PlatformCalibration.mag3D.correctionMatrix.m20 = (float)(Platform.PlatformCalibration.mag.offset.offsetY);
        //Platform.PlatformCalibration.mag3D.correctionMatrix.m30 = (float)(Platform.PlatformCalibration.mag.offset.offsetZ);
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m11 = rotate[0];
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m12 = rotate[1];
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m13 = rotate[2];
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m21 = rotate[3];
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m22 = rotate[4];
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m23 = rotate[5];
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m31 = rotate[6];
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m32 = rotate[7];
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m33 = rotate[8];
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m10 = .0f;
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m20 = .0f;
        pPlatform->PlatformCalibration.mag3D.correctionMatrix.m30 = .0f;

        //SAFE_MEMCPY(&Platform.PlatformCalibration, &Calibration);
    }
    TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "%!FUNC! Exit");
    return pPlatform;
}

void DestroyPlatform(pIvhPlatform pPlatform)
{
    if(pPlatform->cali != NULL)
    {
        DestoryCalibrationData(pPlatform->cali);
        pPlatform->cali = NULL;
    }
    if(pPlatform!= NULL)
    {
        SAFE_FREE_POOL(pPlatform, IVH_FUSION_PLATFORM_POOL_TAG);
        pPlatform= NULL;
    }
}

//------------------------------------------------------------------------------
// Local FUNCTIONS
//------------------------------------------------------------------------------
/****************************************************
=========orientation matrix==========================
x   y   z
-y  x   z
-x  -y  z
y   -x  z
x   -y  -z
y   x   -z
-x  y   -z
-y  -x  -z
*****************************************************/
static void PlatformCalibrationGetOrientation(PLATFORM_CALIBRATION* cali, CALIBRATION_ORIENTATION_STRUCT_T* a, CALIBRATION_ORIENTATION_STRUCT_T* g, CALIBRATION_ORIENTATION_STRUCT_T* m)
{
    // we suppose device can only mount in z-direction
    if ((int)(cali->gyrozz)>GYROMETER_ROTATE_THRESHOLD)
    {
        g->reverseZ = FALSE;
        g->positionOfZ = 1;
    }
    else if ((int)(cali->gyrozz)<-GYROMETER_ROTATE_THRESHOLD)
    {
        g->reverseZ = TRUE;
        g->positionOfZ = 1;
    }

    if ((int)(cali->gyroyx)>GYROMETER_ROTATE_THRESHOLD)
    {
        g->reverseY = FALSE;
        g->positionOfX = 3;
        g->positionOfY = g->positionOfX ^ g->positionOfZ;
    }
    else if ((int)(cali->gyroyx)<-GYROMETER_ROTATE_THRESHOLD)
    {
        g->reverseY = TRUE;
        g->positionOfX = 3;
        g->positionOfY = g->positionOfX ^ g->positionOfZ;
    }
    else if ((int)(cali->gyroyy)>GYROMETER_ROTATE_THRESHOLD)
    {
        g->reverseY = FALSE;
        g->positionOfY = 3;
        g->positionOfX = g->positionOfY ^ g->positionOfZ;
    }
    else if ((int)(cali->gyroyy)<-GYROMETER_ROTATE_THRESHOLD)
    {
        g->reverseY = TRUE;
        g->positionOfY = 3;
        g->positionOfX = g->positionOfY ^ g->positionOfZ;
    }
    g->reverseX = g->reverseY ^ g->reverseZ;
    if(g->positionOfY != 3) //swap
        g->reverseX = !g->reverseX;

    if ((int)(cali->acclzz)>ACCELEROMETER_ROTATE_THRESHOLD)
    {
        a->reverseZ = TRUE;
        a->positionOfZ = 1;
    }
    else if ((int)(cali->acclzz)<-ACCELEROMETER_ROTATE_THRESHOLD)
    {
        a->reverseZ = FALSE;
        a->positionOfZ = 1;
    }

    if ((int)(cali->acclyx)>ACCELEROMETER_ROTATE_THRESHOLD)
    {
        a->reverseY = TRUE;
        a->positionOfX = 3;
        a->positionOfY = a->positionOfX ^ a->positionOfZ;
    }
    else if ((int)(cali->acclyx)<-ACCELEROMETER_ROTATE_THRESHOLD)
    {
        a->reverseY = FALSE;
        a->positionOfX = 3;
        a->positionOfY = a->positionOfX ^ a->positionOfZ;
    }
    else if ((int)(cali->acclyy)>ACCELEROMETER_ROTATE_THRESHOLD)
    {
        a->reverseY = TRUE;
        a->positionOfY = 3;
        a->positionOfX = a->positionOfY ^ a->positionOfZ;
    }
    else if ((int)(cali->acclyy)<-ACCELEROMETER_ROTATE_THRESHOLD)
    {
        a->reverseY = FALSE;
        a->positionOfY = 3;
        a->positionOfX = a->positionOfY ^ a->positionOfZ;
    }
    a->reverseX = a->reverseY ^ a->reverseZ;
    if(a->positionOfY == 3) //swap
        a->reverseX = !a->reverseX;

    m->positionOfX = a->positionOfX;
    m->positionOfY = a->positionOfY;
    m->positionOfZ = a->positionOfZ;
    m->reverseX = a->reverseX ^ TRUE;
    m->reverseY = a->reverseY ^ TRUE;
    m->reverseZ = a->reverseZ ^ TRUE;
}

static PPLATFORM_CALIBRATION CreateCalibrationData(void)
{
    PPLATFORM_CALIBRATION pPlatform = (PPLATFORM_CALIBRATION)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(PLATFORM_CALIBRATION),IVH_FUSION_PLATFORM_POOL_TAG);
    if (pPlatform) 
    {
        // initlize the config
        SAFE_FILL_MEM (pPlatform, sizeof (PLATFORM_CALIBRATION), 0);
        /**set golden config**/
        pPlatform->calibrated = 2;
        pPlatform->spen = 0;
        pPlatform->shake_th = 1800;
        pPlatform->shake_shock = 50;
        pPlatform->shake_quiet = 300;
        pPlatform->accl_tolarence = 20;
        pPlatform->gyro_tolarence = 400;
        pPlatform->magn_tolarence = 50;
        pPlatform->stableMS = 2000;

        ULONG als_curve[20]={12,0,24,0,35,1,47,4,59,10,71,28,82,75,94,206,106,565,118,1547};
        /**set golden config**/
        pPlatform->acclx = (ULONG)(0);
        pPlatform->accly = (ULONG)(0);
        pPlatform->acclz = (ULONG)(0);
        pPlatform->gyrox = (ULONG)(0);
        pPlatform->gyroy = (ULONG)(0);
        pPlatform->gyroz = (ULONG)(0);
        pPlatform->magnnx = (ULONG)(0);
        pPlatform->magnny = (ULONG)(268);
        pPlatform->magnnz = (ULONG)(0);
        pPlatform->magnsx = (ULONG)(0);
        pPlatform->magnsy = (ULONG)(-268);
        pPlatform->magnsz = (ULONG)(0);
#if 1
		pPlatform->magnxnx= (ULONG)(2452);
		pPlatform->magnxny= (ULONG)(-194);
		pPlatform->magnxnz= (ULONG)(-2561);
		
		pPlatform->magnxsx= (ULONG)(540);
		pPlatform->magnxsy= (ULONG)(2332);
		pPlatform->magnxsz= (ULONG)(-7105);
		
		pPlatform->acclzx= (ULONG)(-9);
		pPlatform->acclzy= (ULONG)(-10037);
		pPlatform->acclzz= (ULONG)(-15);
		
		pPlatform->acclyx= (ULONG)(-49);
		pPlatform->acclyy= (ULONG)(-10095);
		pPlatform->acclyz= (ULONG)(-244);

		pPlatform->gyrox = (ULONG)(1196);
		pPlatform->gyroy = (ULONG)(2136);
		pPlatform->gyroz = (ULONG)(-2563);

		pPlatform->gyrozx = (ULONG)(-3738);
		pPlatform->gyrozy = (ULONG)(1570);
		pPlatform->gyrozz = (ULONG)(224);

		pPlatform->gyroyx = (ULONG)(5084);
		pPlatform->gyroyy = (ULONG)(69914);
		pPlatform->gyroyz = (ULONG)(-1682);
		
#endif		
        for(int i = 0; i < sizeof(als_curve)/sizeof(ULONG); i++)
        {
            pPlatform->alscurve[i] = als_curve[i];
        }
        pPlatform->als_multiplier = (ULONG)(1000);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for calibration data");
    }
    else 
    {
        //  No memory
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "not enough memory");
        return NULL;
    }

    ERROR_T status = InitlizeFusionParametersFromFirmware(pPlatform);
    if(ERROR_OK == status)
    {
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "[%!FUNC!] success,calibrate=%u", pPlatform->calibrated);
    }
    else 
    {
        TraceLog(TRACE_LEVEL_WARNING, TRACE_ALGO, "[%!FUNC!] failed! - %!STATUS!", status);
        TraceLog(TRACE_LEVEL_WARNING, TRACE_ALGO, "system is not calibrated!");
    }
    return pPlatform;
}

void DestoryCalibrationData(PPLATFORM_CALIBRATION pPlatform)
{
    if(pPlatform!= NULL)
    {
        SAFE_FREE_POOL(pPlatform, IVH_FUSION_PLATFORM_POOL_TAG);
        pPlatform= NULL;
    }
}

ERROR_T InitlizeFusionParametersFromFirmware(PPLATFORM_CALIBRATION pPlatform)
{
    ERROR_T status = ERROR_OK;
/********cjy
    LPGUID guid = (LPGUID) &IVSFHUBHIDMINI_VARIABLE_GUID;
    ULONG length = 0;
    UNICODE_STRING string;
    char buf[sizeof(PLATFORM_CALIBRATION)];

    RtlInitUnicodeString(&string, L"nvram");

    length = sizeof(buf);
    status = ExGetFirmwareEnvironmentVariable(&string, guid, buf, &length, NULL);
    if (ERROR_OK != status)
    {
        TraceLog(TRACE_LEVEL_WARNING, TRACE_ALGO, "Error read calibration data. - %!STATUS!", status);
    }
    // load parameters...
    else if(length == sizeof(buf))
    {
        RtlMoveMemory(pPlatform, buf, sizeof(PLATFORM_CALIBRATION));
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "read firmware data success");
    }
    *******/
    return status;
}
