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
// Error.h : Base error codes
//

#if !defined(_ERROR_H_)
#define _ERROR_H_

#include <stdint.h>      // for uint16_t

//------------------------------------------
// CONSTANTS
//------------------------------------------

#define ERROR_OK                0x00
#define ERROR_BAD_POINTER       0x01
#define ERROR_WRITE_FAILED      0x02
#define ERROR_BAD_VALUE         0x03
#define ERROR_INVALID_REPORT_ID 0x04
#define ERROR_UNKNOWN           0x05
#define ERROR_READ_ONLY         0x06
#define ERROR_LAST              ERROR_UNKNOWN

/**
TYPEDEFS
*/

typedef uint16_t ERROR_T;

#endif /* !defined(_ERROR_H_) */

