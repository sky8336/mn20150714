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
//
//
////////////////////////////////////////////////////////////////////////////////

//
// Conversion.cpp : Centralized place for all conversion routines
//
/* Includes ------------------------------------------------------------------*/
#include "Common.h"             // for IN
#include "Error.h"              // for ERROR_T
#include "math.h"               // for M_PI

//------------------------------------------------------------------------------
// FUNCTIONS
//------------------------------------------------------------------------------
float ConversionNormalizeDegrees(const float degrees);
float ConversionRadiansToDegrees(const float radians);
float ConversionDegreesToRadians(const float degrees);

// Convert degrees into a range 0-359
// Handles negative degrees and also handles values over or under 360.
// ex. 720 is normalized to 0.
float ConversionNormalizeDegrees(const float degrees)
{
    // find the number of times the input value is evenly divisible by 360
    // if input degrees are negative, we'll get a negative scale factor
    int16_t scaleFactor = (int16_t)degrees / 360;

    // Now reduce the value to somewhere between +- 360 degrees
    float normalizedDegrees = degrees - (360 * scaleFactor);

    // If degrees are negative, convert it to positive 0-360
    // ex.  -90 degrees == +270 degrees
    if (0 > normalizedDegrees)
    {
        normalizedDegrees = 360 + degrees;
    }
    return normalizedDegrees;
}

float ConversionRadiansToDegrees(const float radians)
{
    return (radians * 180 / M_PI);
}

float ConversionDegreesToRadians(const float degrees)
{
    return (degrees * M_PI / 180);
}
