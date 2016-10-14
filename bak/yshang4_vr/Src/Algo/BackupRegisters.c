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

#include "BackupRegisters.h"
#include <stddef.h>
#include <string.h>
#include "Common.h"

#include "Trace.h"

// Array that maps backup register enumeration to backup register address
static uint16_t  gs_backupRegisters[128];

#define NUMBER_BACKUP_REGISTERS (sizeof(gs_backupRegisters)/sizeof(gs_backupRegisters[0]))

// Array of indices of essential backup registers - i.e., backup registers that
// will not be cleared when BackupRegistersClearNonEssential() is called
static const BACKUP_REGISTER_T gs_essentialBackupRegisters[] = {
    BKP_BOOT_COUNT
};
#define NUMBER_ESSENTIAL_BACKUP_REGISTERS \
    (sizeof(gs_essentialBackupRegisters)/sizeof(gs_essentialBackupRegisters[0]))

// Helper function declarations

// Determines if a backup register has been "marked" as essential
static BOOLEAN BackupRegisterIsEssential(
    BACKUP_REGISTER_T br
    );
#if 0
#define IVH_BACKUP_POOL_TAG 'BHVI'

static PRKDPC dpc;
static PKTIMER timer;

//TODO: we restore backup settings when driver loading and store to registry when driver unloading
//if we need store anytime, we need enable ENABLE_TIMER_DPC feature...

//#define ENABLE_TIMER_DPC

static NTSTATUS ReadBackupRegistersFromRegistry(HANDLE_DEV Device);
static NTSTATUS WriteBackupRegistersToRegistry(HANDLE_DEV Device);

#ifdef ENABLE_TIMER_DPC
static VOID TimerHandle(PRKDPC dpc, PVOID arg1, PVOID arg2, PVOID arg3)
{
    UNREFERENCED_PARAMETER(dpc);
    UNREFERENCED_PARAMETER(arg2);
    UNREFERENCED_PARAMETER(arg3);
    WriteBackupRegistersFromRegistry(static_cast<HANDLE_DEV>(arg1));
}
#endif
// Public interface
void BackupRegistersInit(HANDLE_DEV dev)
{
    memset(gs_backupRegisters, BKP_UNINITIALIZED_VALUE, sizeof(gs_backupRegisters));
    ReadBackupRegistersFromRegistry(dev);

#ifdef ENABLE_TIMER_DPC
    LARGE_INTEGER due;
    due.QuadPart = 10000*10000UL;
    dpc = (PRKDPC)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(KDPC), IVH_BACKUP_POOL_TAG);

    if (dpc)
    {   // init it!
        KeInitializeDpc(dpc, TimerHandle, dev);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for backup dpc");
    }
    else 
    {
        //  No memory
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]not enough memory");
        return;
    }

    timer = (PKTIMER)SAFE_ALLOCATE_POOL(NonPagedPool, sizeof(KTIMER), IVH_BACKUP_POOL_TAG);
    if (timer)
    {   // init it!
        KeInitializeTimer(timer);
        TraceLog(TRACE_LEVEL_INFORMATION, TRACE_ALGO, "malloc memory for backup timer");
    }
    else 
    {
        //  No memory
        TraceLog(TRACE_LEVEL_ERROR, TRACE_ALGO, "[%!FUNC!]not enough memory");
        return;
    }

    KeSetTimerEx(timer, due, (ULONG)(due.QuadPart), dpc);
#endif
}

void BackupRegistersDeInit(HANDLE_DEV dev)
{
    WriteBackupRegistersToRegistry(dev);
#ifdef ENABLE_TIMER_DPC
    KeCancelTimer(timer);
    KeFlushQueuedDpcs(); //wait for dpc complete
    SAFE_FREE_POOL(dpc, IVH_BACKUP_POOL_TAG);
    SAFE_FREE_POOL(timer, IVH_BACKUP_POOL_TAG);
#endif
}
#endif
uint16_t BackupRegisterRead(BACKUP_REGISTER_T br)
{
    return br < NUMBER_BACKUP_REGISTERS ?
        gs_backupRegisters[br] : BKP_UNINITIALIZED_VALUE;
}

void BackupRegisterWrite(BACKUP_REGISTER_T br, uint16_t val)
{
    if (br < NUMBER_BACKUP_REGISTERS)
    {
        gs_backupRegisters[br]= val;
    }
}

void BackupRegistersClearAllNonEssential(void)
{
    // Walk through all of the backup registers
    size_t index;
    for (index = 0; index < NUMBER_BACKUP_REGISTERS; ++index)
    {
        // If the backup regsiter is not essential, clear it.
        if (BackupRegisterIsEssential((BACKUP_REGISTER_T)(index)) == FALSE)
        {
            BackupRegisterClear((BACKUP_REGISTER_T)(index));
        }
    }
} // BackupRegistersClearAllNonEssential

// Private implementation

BOOLEAN BackupRegisterIsEssential(
    BACKUP_REGISTER_T br
    )
{
    // Run through all of the essential registers, looking for this one.
    size_t essential;
    for (essential = 0; essential < NUMBER_ESSENTIAL_BACKUP_REGISTERS; ++essential)
    {
        if (br == gs_essentialBackupRegisters[essential])
        {
            break;
        }
    }

    // If we reached the end of the list, it is a non-essential register.  Otherwise
    // it is essential.
    return NUMBER_ESSENTIAL_BACKUP_REGISTERS <= essential ? FALSE : TRUE;
} // BackupRegisterIsEssential



// x is not really a char pointer, it contains the bits that should 
// be split and written into the two supplied backup registers
void BackupRegisterWrite32Bits(IN const BACKUP_REGISTER_T bkp1,
                               IN const BACKUP_REGISTER_T bkp2,
                               IN const char* x)
{
    uint16_t firstHalf =  *((uint16_t *)x);
    uint16_t secondHalf = *((uint16_t *)(x + 2));
    BackupRegisterWrite(bkp1, firstHalf);
    BackupRegisterWrite(bkp2, secondHalf);
}

void BackupRegisterRead32Bits(IN const BACKUP_REGISTER_T bkp1,
                              IN const BACKUP_REGISTER_T bkp2,
                              OUT char* const x)
{
    uint16_t firstHalf = BackupRegisterRead(bkp1);
    uint16_t secondHalf = BackupRegisterRead(bkp2);
    memcpy((char *)x, &firstHalf, 2);
    memcpy(((char *)x) + 2, &secondHalf, 2);
}

void BackupRegisterWriteFloat(IN const BACKUP_REGISTER_T bkp1,
                              IN const BACKUP_REGISTER_T bkp2,
                              IN const float x)
{
    BackupRegisterWrite32Bits(bkp1, bkp2, (char *)&x);
}

void BackupRegisterReadFloat(IN const BACKUP_REGISTER_T bkp1,
                             IN const BACKUP_REGISTER_T bkp2,
                             OUT float* const x)
{
    BackupRegisterRead32Bits(bkp1, bkp2, (char *)x);
}

void BackupRegisterWriteInt32(IN const BACKUP_REGISTER_T bkp1,
                              IN const BACKUP_REGISTER_T bkp2,
                              IN const int32_t x)
{
    BackupRegisterWrite32Bits(bkp1, bkp2, (char *)&x);
}

void BackupRegisterReadInt32(IN const BACKUP_REGISTER_T bkp1,
                             IN const BACKUP_REGISTER_T bkp2,
                             OUT const int32_t* x)
{
    BackupRegisterRead32Bits(bkp1, bkp2, (char *)x);
}
#if 0
NTSTATUS ReadBackupRegistersFromRegistry(HANDLE_DEV Device)
{
    WDFKEY key = NULL;
    WDFKEY subkey = NULL;
    NTSTATUS status;

    // Retrieve registry settings.
    DECLARE_CONST_UNICODE_STRING(subConfigName, L"Config");
    DECLARE_CONST_UNICODE_STRING(backupName, L"backup");

    status = WdfDeviceOpenRegistryKey(Device, PLUGPLAY_REGKEY_DEVICE, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &key);

    if (!NT_SUCCESS(status))
    {
        TraceLog(TRACE_LEVEL_WARNING, TRACE_ALGO, "Error opening device registry key - %!STATUS!", status);
    }
    else
    {
        status = WdfRegistryOpenKey(key, &subConfigName, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &subkey);

        if (!NT_SUCCESS(status))
        {
            TraceLog(TRACE_LEVEL_WARNING, TRACE_ALGO, "Error opening registry subkey for 'Config' - %!STATUS!", status);
        }
        else
        {
            ULONG length = sizeof(gs_backupRegisters);
            status = WdfRegistryQueryValue(subkey, &backupName, length, gs_backupRegisters, &length, NULL);

            if (!NT_SUCCESS(status))
            {
                TraceLog(TRACE_LEVEL_WARNING, TRACE_ALGO, "Error querying registry value for 'backup reg' - %!STATUS!", status);
            }
        }
        if (subkey != NULL)
        {
            WdfRegistryClose(subkey);
            subkey = NULL;
        }
    }
    if (key != NULL)
    {
        WdfRegistryClose(key);
        key = NULL;
    }
    return status;
}

NTSTATUS WriteBackupRegistersToRegistry(HANDLE_DEV Device)
{
    WDFKEY key = NULL;
    WDFKEY subkey = NULL;
    NTSTATUS status;

    // Retrieve registry settings.
    DECLARE_CONST_UNICODE_STRING(subConfigName, L"Config");
    DECLARE_CONST_UNICODE_STRING(backupName, L"backup");

    status = WdfDeviceOpenRegistryKey(Device, PLUGPLAY_REGKEY_DEVICE, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &key);

    if (!NT_SUCCESS(status))
    {
        TraceLog(TRACE_LEVEL_WARNING, TRACE_ALGO, "Error opening device registry key - %!STATUS!", status);
    }
    else
    {
        status = WdfRegistryOpenKey(key, &subConfigName, KEY_READ, WDF_NO_OBJECT_ATTRIBUTES, &subkey);

        if (!NT_SUCCESS(status))
        {
            TraceLog(TRACE_LEVEL_WARNING, TRACE_ALGO, "Error opening registry subkey for 'Config' - %!STATUS!", status);
        }
        else
        {
            ULONG length = sizeof(gs_backupRegisters);
            status = WdfRegistryAssignValue(subkey, &backupName, REG_BINARY, length, gs_backupRegisters);

            if (!NT_SUCCESS(status))
            {
                TraceLog(TRACE_LEVEL_WARNING, TRACE_ALGO, "Error querying registry value for 'backup reg' - %!STATUS!", status);
            }
        }
        if (subkey != NULL)
        {
            WdfRegistryClose(subkey);
            subkey = NULL;
        }
    }
    if (key != NULL)
    {
        WdfRegistryClose(key);
        key = NULL;
    }
    return status;
}
#endif
