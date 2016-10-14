////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2011, Intel Corporation.  All Rights Reserved.       //
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

#if !defined(__PLATFORM_FUSION_H__)
#define __PLATFORM_FUSION_H__

typedef struct _FUSION_CONFIG
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
} FUSION_CONFIG, *PFUSION_CONFIG;

typedef struct _FUSION_CALIBRATION
{
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
} FUSION_CALIBRATION, *PFUSION_CALIBRATION;


#endif //__PLATFORM_FUSION_H__