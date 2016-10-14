////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2013, Intel Corporation.  All Rights Reserved.            //
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
#include "Vector.h"
#include <stdlib.h>

void Vector3I32MultiplyByScalar(
    INOUT               INT32_VECTOR3_T*    const   pV3I32,
    IN      const       int32_t                     scalar)
{
    pV3I32->x *= scalar;
    pV3I32->y *= scalar;
    pV3I32->z *= scalar;
}

void Vector3I32DivideByScalar(
    INOUT               INT32_VECTOR3_T*    const   pV3I32,
    IN      const       int32_t                     scalar)
{
    pV3I32->x /= scalar;
    pV3I32->y /= scalar;
    pV3I32->z /= scalar;
}

void Vector3I32Subtract(
    IN  const       INT32_VECTOR3_T*    __restrict  pMinuend,
    IN  const       INT32_VECTOR3_T*    __restrict  pSubtrahend,
    IN              INT32_VECTOR3_T*    __restrict  pDifference)
{
    pDifference->x = pMinuend->x - pSubtrahend->x;
    pDifference->y = pMinuend->y - pSubtrahend->y;
    pDifference->z = pMinuend->z - pSubtrahend->z;
}

static BOOLEAN Vector3I32AnyElementMagnitudeGteScalar(
    IN              INT32_VECTOR3_T*    const   pV3I32,
    IN  const       int32_t                     scalar)
{
    return ((abs(pV3I32->x) > scalar) ||
        (abs(pV3I32->y) > scalar) ||
        (abs(pV3I32->z) > scalar));
}

BOOLEAN Vector3I32AnyDeltaElementMagnitudeExceedsScalar(
    IN  const       INT32_VECTOR3_T*    __restrict  pFirstVector,
    IN  const       INT32_VECTOR3_T*    __restrict  pSecondVector,
    IN  const       uint32_t                        scalar)
{
    INT32_VECTOR3_T difference;
    Vector3I32Subtract(pFirstVector, pSecondVector, &difference);
    return Vector3I32AnyElementMagnitudeGteScalar(&difference, scalar);
}
