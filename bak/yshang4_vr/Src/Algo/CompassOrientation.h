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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _COMPASS_ORIENTATION_H_
#define _COMPASS_ORIENTATION_H_

#include "SensorManagerTypes.h"
#include "Common.h"
#include "Error.h"
#include "platformCalibration.h"
#include "./Sensor.h"

//------------------------------------------------------------------------------
// DATA
//------------------------------------------------------------------------------
typedef enum
{ 
    AXIS_REFERENCE_POSITIVE_Y,
    AXIS_REFERENCE_NEGATIVE_Y,
    AXIS_REFERENCE_NEGATIVE_Z,
    AXIS_REFERENCE_POSITIVE_Z,
} AXIS_REFERENCE_T;

typedef struct _IVH_SENSOR_COMPASS_T
{
    IvhSensor sensor; //keep me first!
    IvhSensorData input;
    IvhSensorData output;

    float lastAxisReferenceChangePitchDegrees;
    BOOLEAN s_axisChangeAllowed;

    AXIS_REFERENCE_T axisReference;
}IvhSensorCompass, *pIvhSensorCompass;

pIvhSensor CreateSensorCompass(pIvhSensor orientation);
void DestroySensorCompass(pIvhSensor me);

#endif /* _COMPASS_ORIENTATION_H_ */


