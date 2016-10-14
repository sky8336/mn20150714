////////////////////////////////////////////////////////////////////////////////
//    Copyright (c) 2007-2013, Intel Corporation.  All Rights Reserved.	      //
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
// distributed, or disclosed in any way without Intel's prior express written //
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
#include "Matrix.h"

void MatrixMultiply(IN const MATRIX_STRUCT_T* const matrixA,
                    IN const MATRIX_STRUCT_T* const matrixB,
                    OUT MATRIX_STRUCT_T* const matrixOut)
{
    ASSERT(matrixA->cols == matrixB->rows);
    ASSERT(matrixOut->rows == matrixB->rows);
    ASSERT(matrixOut->cols == matrixB->cols);

    int16_t i, j, k;
    float sum;
    for (i=0; i < matrixA->rows; i++)
    {
        for (j=0; j < matrixB->cols; j++)
        {
            sum = 0;
            for (k=0; k < matrixOut->rows; k++)
            {
                sum += matrixA->m[i * matrixA->cols + k] *
                    matrixB->m[k * matrixB->cols + j];
            }
            matrixOut->m[i * matrixOut->cols + j] = sum;
        }
    }
}

void MatrixAdd(IN const MATRIX_STRUCT_T* const matrixA,
               IN const MATRIX_STRUCT_T* const matrixB,
               OUT MATRIX_STRUCT_T* const matrixOut)
{
    ASSERT(matrixA->rows == matrixB->rows);
    ASSERT(matrixA->cols == matrixB->cols);
    ASSERT(matrixOut->rows == matrixA->rows);
    ASSERT(matrixOut->cols == matrixA->cols);

    int16_t i, j;
    for (i=0; i < matrixOut->rows;  i++)
    {
        for (j=0; j < matrixOut->cols; j++)
        {
            int16_t index = i * matrixOut->cols + j;
            matrixOut->m[index] =  matrixA->m[index] + matrixB->m[index];
        }
    }       
}
