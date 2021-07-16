#pragma once
#include <ntdef.h>
#include <ntifs.h>
#include <ntddk.h>
#include "driver_io.h"
#include <VMProtectDDK.h>

#ifdef DBG
#define DPRINT(format, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, format, __VA_ARGS__)
#else
#define DPRINT(...)
#endif

extern PDEVICE_OBJECT pDeviceObj;

extern UNICODE_STRING DeviceName;
extern UNICODE_STRING DosName;


//FOR MY PROCESS PROTECT
extern HANDLE PROTECTED_PROCESS;
extern PEPROCESS PEPROTECTED_PROCESS;


//FOR GAME CHEAT
extern HANDLE GAME_PROCESS;
extern PEPROCESS PEGAME_PROCESS;

//CS GO MODULES
extern DWORD32 CLIENT_DLL_BASE;
extern DWORD32 ENGINE_DLL_BASE;

//DATA INIT
extern BOOLEAN DRIVER_INITED;

extern PDRIVER_OBJECT g_Driver;

typedef NTKERNELAPI NTSTATUS(NTAPI* MmCopyVirtualMemoryFn)(
    IN PEPROCESS FromProcess,
    IN PVOID FromAddress,
    IN PEPROCESS ToProcess,
    OUT PVOID ToAddress,
    IN SIZE_T BufferSize,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PSIZE_T NumberOfBytesCopied
    );

extern UNICODE_STRING MemCopyRoutineName;
extern MmCopyVirtualMemoryFn MemCopy;
