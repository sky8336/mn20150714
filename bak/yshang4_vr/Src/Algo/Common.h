////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2013, Intel Corporation.  All Rights Reserved.	      //
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

//
// Common.h : Common types and #defines used across the entire project
//

#if!defined(_COMMON_H_)
#define _COMMON_H_

#include <stdint.h>
#include "stdintext.h"
#include <string.h>
#include <stdlib.h>
//------------------------------------------
// TYPES
//------------------------------------------
#if !defined(INOUT)
#define INOUT
#endif /* !defined(INOUT) */

#if !defined(IN)
#define IN
#endif /* !defined(IN) */

#if !defined(OUT)
#define OUT
#endif /* !defined(OUT) */

#if !defined(__restrict)
#define __restrict // restrict
#endif /* !defined(__restrict) */

#if !defined(SIZE_T)
#define SIZE_T size_t
#endif /* !defined(SIZE_T) */


extern void TraceLogByUart( uint8_t * data);
extern char dataOut[256];

#ifdef DEBUG
#define ASSERT(b) do{if(!b);}while(0);
#define TraceLog(level, tag, ...) do{sprintf( dataOut, ...);TraceLogByUart(dataOut)}while(0);
#else
#define ASSERT(b)
#define TraceLog(level, tag, ...)
#endif

//------------------------------------------
// CONSTANTS
//------------------------------------------

#ifndef NULL
#define NULL						0
#endif

#if !defined(FALSE)
#define FALSE						0
#endif
#if !defined(TRUE)
#define TRUE						(~FALSE)    // will be 1
#endif
#define UNKNOWN                     2

#if !defined(_PACKED_)
#define _PACKED_                    attribute ((__packed__))
#endif

#define MIN(a,b)                    (((a) > (b)) ? (b) : (a))
#define MAX(a,b)                    (((a) < (b)) ? (b) : (a))

// A handy macro for getting rid of unreferenced parameter or variable warnings
#define UNREFERENCED_LOCAL(l)       ((void) l)
#if !defined(UNREFERENCED_PARAMETER)
#define UNREFERENCED_PARAMETER(p)   UNREFERENCED_LOCAL(p)
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

//------------------------------------------
// STRUCT / DATA DECLARATIONS
//------------------------------------------

typedef enum
{
    AXIS_X = 0,
    AXIS_Y,
    AXIS_Z,
    NUM_AXES
} AXIS_T;

#define COVARIANCE_MATRIX_SIZE 6

// The following two macros always use the size of the type of *dest as the number
// of bytes to copy/set.  Do not use them if you intend to copy a # of 
// bytes that is smaller than the type of *dest.  The other case (where
// the intent is to copy a # of bytes that is larger than the # of bytes of 
// the type of *dest) is a memory overflow and should never be done.
#define SAFE_MEMCPY(dest, source)       memcpy(dest, source, sizeof(*(dest)))
#define SAFE_MEMSET(dest, value)        memset(dest, value, sizeof(*(dest)))
#define SAFE_MEMCPY_BYTES(dest, source, size)       memcpy(dest, source, size)
#define SAFE_ZERO_MEM(dest, size)        memset(dest, 0, size)
#define SAFE_FILL_MEM(dest, size, value)        memset(dest, value, size)
#define SAFE_FREE_POOL(dest, tag) free(dest)
#define SAFE_ALLOCATE_POOL(type, size, tag) malloc(size)

//#define isfinite ipp_finite_32f
int ipp_isnan_32f( float x );
int ipp_finite_32f( float x );

#endif /* !defined(_COMMON_H_) */

