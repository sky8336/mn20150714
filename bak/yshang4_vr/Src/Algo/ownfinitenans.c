/* ////////////////////////// ownfinitenans.c ////////////////////////// */
/* 
//
//              INTEL CORPORATION PROPRIETARY INFORMATION
//  This software is supplied under the terms of a license  agreement or
//  nondisclosure agreement with Intel Corporation and may not be copied
//  or disclosed except in  accordance  with the terms of that agreement.
//     Copyright (c) 2010 Intel Corporation. All Rights Reserved.
//
// Created: Tuesday, April 20, 2010 8:50:02 AM by asolntse
//
*/

/*=======================================================================*/
int ipp_isnan_32f( float x );
int ipp_finite_32f( float x );

int ipp_isnan_32f( float x )
{
    int *ix = (int*)(&(x));

    if( (*ix & 0x7f800000) == 0x7f800000 ) {
        if( !(*ix & 0x00400000) ) {
            if( (*ix & 0x003fffff) ) {
                return 1;
            }
        } else {
            return 1;
        }
    }

    return 0;
}

/*=======================================================================*/
int ipp_finite_32f( float x )
{
    int *ix = (int*)(&(x));

    if( (*ix & 0x7f800000) == 0x7f800000 ) {
        return 0;
    }

    return 1;
}