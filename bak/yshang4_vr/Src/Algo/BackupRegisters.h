////////////////////////////////////////////////////////////////////////////////
//      Copyright (c) 2012, Intel Corporation.  All Rights Reserved.          //
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
#if !defined(_BACKUP_REGISTERS_H_)
#define _BACKUP_REGISTERS_H_


#include "SensorManagerTypes.h"
// 0x0000 is the default value of the backup registers if vBatt is removed
#define BKP_UNINITIALIZED_VALUE 0x0000

// NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE
//
// Here is where you add new backup registers that are used.  If you add a new
// backup register that should not be cleared when 
// BackupRegistersCleanAllNonEssential() is called, go to BackupRegisters.c and
// add the enumeration value to the array of essential backup register indices.
//
// NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE - NOTE
typedef enum {
    BKP_DFU_ENABLE,
    BKP_DFU_COUNT,
    BKP_MAGIC_SALT,
    BKP_CALIBRATION_CENTROID_X_A,
    BKP_CALIBRATION_CENTROID_Y_A,
    BKP_CALIBRATION_CENTROID_Z_A,
    BKP_CALIBRATION_RADIUS_ELLIPSOID,
    BKP_FREE_FALL_COUNT,
    BKP_BOOT_COUNT,
    BKP_USB_NON_DESTRUCTIVE_RESETS,
    BKP_CALIBRATION_CENTROID_X_B,
    BKP_CALIBRATION_CENTROID_Y_B,
    BKP_CALIBRATION_CENTROID_Z_B,
    BKP_RMAT_M11,
    BKP_RMAT_M12,
    BKP_RMAT_M13,
    BKP_RMAT_M21,
    BKP_RMAT_M22,
    BKP_RMAT_M23,
    BKP_RMAT_M31,
    BKP_RMAT_M32,
    BKP_RMAT_M33,
    BKP_GYRO_OFFSET_X_A,
    BKP_GYRO_OFFSET_X_B,
    BKP_GYRO_OFFSET_Y_A,
    BKP_GYRO_OFFSET_Y_B,
    BKP_GYRO_OFFSET_Z_A,
    BKP_GYRO_OFFSET_Z_B,
    BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_X_A,
    BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_X_B,
    BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Y_A,
    BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Y_B,
    BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Z_A,
    BKP_ACCUMULATED_DEGREES_SINCE_GOOD_MAG_Z_B,

    BKP_ERROR_FIFO_HEAD,
    BKP_ERROR_FIFO_END,
    BKP_ERROR_FIFO_SAMPLES,
    BKP_ERROR_FIFO_ERROR,
    BKP_ERROR_FIFO_X0,
    BKP_ERROR_FIFO_Y0,
    BKP_ERROR_FIFO_Z0,
    BKP_ERROR_FIFO_X1,
    BKP_ERROR_FIFO_Y1,
    BKP_ERROR_FIFO_Z1,
    BKP_ERROR_FIFO_X2,
    BKP_ERROR_FIFO_Y2,
    BKP_ERROR_FIFO_Z2,
    BKP_ERROR_FIFO_X3,
    BKP_ERROR_FIFO_Y3,
    BKP_ERROR_FIFO_Z3,
    BKP_ERROR_FIFO_X4,
    BKP_ERROR_FIFO_Y4,
    BKP_ERROR_FIFO_Z4,
    BKP_ERROR_FIFO_X5,
    BKP_ERROR_FIFO_Y5,
    BKP_ERROR_FIFO_Z5,

    BKP_CALIBRATION_CENTROID_ACC_X_A,
    BKP_CALIBRATION_CENTROID_ACC_Y_A,
    BKP_CALIBRATION_CENTROID_ACC_Z_A,
    BKP_CALIBRATION_CENTROID_ACC_X_B,
    BKP_CALIBRATION_CENTROID_ACC_Y_B,
    BKP_CALIBRATION_CENTROID_ACC_Z_B,

    BKP_ERROR_FIFO_ACC_HEAD,
    BKP_ERROR_FIFO_ACC_END,
    BKP_ERROR_FIFO_ACC_SAMPLES,
    BKP_ERROR_FIFO_ACC_ERROR,
    BKP_ERROR_FIFO_ACC_X0,
    BKP_ERROR_FIFO_ACC_Y0,
    BKP_ERROR_FIFO_ACC_Z0,
    BKP_ERROR_FIFO_ACC_X1,
    BKP_ERROR_FIFO_ACC_Y1,
    BKP_ERROR_FIFO_ACC_Z1,
    BKP_ERROR_FIFO_ACC_X2,
    BKP_ERROR_FIFO_ACC_Y2,
    BKP_ERROR_FIFO_ACC_Z2,
    BKP_ERROR_FIFO_ACC_X3,
    BKP_ERROR_FIFO_ACC_Y3,
    BKP_ERROR_FIFO_ACC_Z3,
    BKP_ERROR_FIFO_ACC_X4,
    BKP_ERROR_FIFO_ACC_Y4,
    BKP_ERROR_FIFO_ACC_Z4,
    BKP_ERROR_FIFO_ACC_X5,
    BKP_ERROR_FIFO_ACC_Y5,
    BKP_ERROR_FIFO_ACC_Z5,
} BACKUP_REGISTER_T;

typedef struct _IVH_BACKUP
{
    uint16_t backupRegisters[128];
}IvhBack, *pIvhBack;

/* NOTE:
*  If using backup registers for storing trace dump information across resets 
*  of the sensor hub then avoid using the following registers:
*      BKP_DR1     : Is the DFU enable registers, the sensor hub will come back 
*                    as a DFU device on reset if this is set.  Avoid writing to 
*                    it. If you desperately need the extra register you can 
*                    erase the DFU section of the flash to force the Root code 
*                    to skip it.
*      BKP_DR3     : Is the MAGIC SALT register and will be written to in main.
*      BKP_DR4-7
*      BKP_DR10-21 : DO NOT USE. If anybody tries to use these they will be
*                    immediately overwritten by DeviceOrientation and 
*                    MagDynamicCali.
*      BKP_DR9     : Is the RTC Initialized register and is written to at init 
*                    time.
*      BKP_DR10    : Is the NUM NON DESTRUCTIVE RESETS register and might be 
*                    written to if USB is mis-behaving.  Use only if you really 
*                    need one extra register.
*      BKP_DR22-28  : Are gyro offsets calculated at runtime
*/

void BackupRegistersInit(HANDLE_DEV dev);
void BackupRegistersDeInit(HANDLE_DEV dev);
uint16_t BackupRegisterRead(BACKUP_REGISTER_T br);
void BackupRegisterWrite(BACKUP_REGISTER_T br, uint16_t val);
void BackupRegistersClearAllNonEssential(void);

// BackupRegister equals BackupRegister + 1
#define BackupRegisterIncrement(br)                 \
    BackupRegisterWrite(br,BackupRegisterRead(br)+1)
// Like normal increment, but used when you care about race conditions.  It also
// will "return" the pre-incremented value.
#define BackupRegisterInterlockedIncrement(br)      \
    ({                                          \
    int16_t ov;                                 \
    INTDisableInterrupts();                     \
    ov = BackupRegisterRead(br);                \
    BackupRegisterWrite(br, ov + 1);            \
    INTEnableInterrupts();                      \
    ov;                                         \
})

// BackupRegister equals BackupRegister - 1
#define BackupRegisterDecrement(br)                 \
    BackupRegisterWrite(br,BackupRegisterRead(br)-1)
// Like normal decrement, but used when you care about race conditions.  It also
// will "return" the pre-decremented value.
#define BackupRegisterInterlockedDecrement(br)      \
    ({                                          \
    int16_t ov;                                 \
    INTDisableInterrupts();                     \
    ov = BackupRegisterRead(br);                \
    BackupRegisterWrite(br, ov - 1);            \
    INTEnableInterrupts();                      \
    ov;                                         \
})

// Clear a backup register - basically set it back to uninitialized
#define BackupRegisterClear(br)                     \
    BackupRegisterWrite(br, BKP_UNINITIALIZED_VALUE)


void BackupRegisterWrite32Bits(IN const BACKUP_REGISTER_T bkp1,
                               IN const BACKUP_REGISTER_T bkp2,
                               IN const char* x);

void BackupRegisterRead32Bits(IN const BACKUP_REGISTER_T bkp1,
                              IN const BACKUP_REGISTER_T bkp2,
                              OUT char* const x);

void BackupRegisterWriteFloat(IN const BACKUP_REGISTER_T bkp1,
                              IN const BACKUP_REGISTER_T bkp2,
                              IN const float x);

void BackupRegisterReadFloat(IN const BACKUP_REGISTER_T bkp1,
                             IN const BACKUP_REGISTER_T bkp2,
                             OUT float* const x);

void BackupRegisterWriteInt32(IN const BACKUP_REGISTER_T bkp1,
                              IN const BACKUP_REGISTER_T bkp2,
                              IN const int32_t x);

void BackupRegisterReadInt32(IN const BACKUP_REGISTER_T bkp1,
                             IN const BACKUP_REGISTER_T bkp2,
                             OUT const int32_t* x);

#endif /* !defined(_BACKUP_REGISTERS_H_) */
