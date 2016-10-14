/******************************************************************************
 *   Copyright (c) 2013, Intel Corporation.  All Rights Reserved.             *
 *                                                                            *
 *              INTEL CORPORATION PROPRIETARY INFORMATION                     *
 *                                                                            *
 * The source code contained or described herein and all documents related to *
 * the source code (Material) are owned by Intel Corporation or its suppliers *
 * or licensors. Title to the Material remains with Intel Corporation or its  *
 * suppliers and licensors. The Material contains trade secrets and           *
 * proprietary and confidential information of Intel or its suppliers and     *
 * licensors. The Material is protected by worldwide copyright and trade      *
 * secret laws and treaty provisions. No part of the Material may be used,    *
 * copied, reproduced, modified, published, uploaded, posted, transmitted,    *
 * distributed, or disclosed in any way without Intel’s prior express written *
 * permission.                                                                *
 *                                                                            *
 * No license under any patent, copyright, trade secret or other intellectual *
 * property right is granted to or conferred upon you by disclosure or        *
 * delivery of the Materials, either expressly, by implication, inducement,   *
 * estoppel or otherwise. Any license under such intellectual property rights *
 * must be express and approved by Intel in writing.                          *
 *                                                                            *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY      *
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE        *
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR      *
 * PURPOSE.                                                                   *
 *                                                                            *
 ******************************************************************************/

#ifndef _STDINTEXT_H
#define _STDINTEXT_H

//#include <basetsd.h>
#ifdef __cplusplus
extern "C"
{
#endif

#ifdef EFI_DEBUG
#define DLOG(fmt, ...) Print(fmt, __VA_ARGS__)
#else
#define DLOG(fmt, ...)
#endif

//typedef long long int64_t;

//typedef unsigned long long uint64_t;

typedef unsigned int	BOOLEAN;
typedef unsigned int	BOOL;
typedef char	BYTE;
typedef unsigned long*	PULONG;
typedef unsigned long	ULONG;
typedef const char*	GUID;
typedef GUID* LPGUID;
typedef void (*ALGO_NOTIFICATION)(ULONG SensorType, void* event);
typedef ULONG (*ALGO_HRT_GET_TS)(void);
typedef struct _HANDLE_DEV
{
    ALGO_NOTIFICATION AlgoNotication;
    ALGO_HRT_GET_TS AlgoHrtGetTs;
}HANDLE_DEV;
#ifdef __cplusplus
}
#endif

#endif //_STDINTEXT_H
