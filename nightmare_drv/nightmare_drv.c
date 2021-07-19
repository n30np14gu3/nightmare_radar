#include "globals.h"
#include "global_defs.h"
#include "functions.h"

#include "ImageLoadCallback.h"
#include "CreateProcessCallback.h"

#include "HookSetup.h"

PDEVICE_OBJECT pDeviceObj;

UNICODE_STRING DeviceName;
UNICODE_STRING DosName;


//Cheat Process
HANDLE PROTECTED_PROCESS;
PEPROCESS PEPROTECTED_PROCESS;

//CSGO Process
HANDLE GAME_PROCESS;
PEPROCESS PEGAME_PROCESS;

//CSGO Modules
DWORD32 CLIENT_DLL_BASE;
DWORD32 ENGINE_DLL_BASE;

BOOLEAN DRIVER_INITED;

PDRIVER_OBJECT g_Driver;

//Routines
MmCopyVirtualMemoryFn MemCopy;
UNICODE_STRING MemCopyRoutineName;

NTSTATUS DriverEntry(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DPRINT("[NIGHTMARE DRV] Loading...");

	DriverObject->DriverUnload = UnloadDriver;


	for (ULONG t = 0; t < IRP_MJ_MAXIMUM_FUNCTION; t++)
		DriverObject->MajorFunction[t] = UnsupportedDispatch;
	
	DPRINT("[NIGHTMARE DRV] Setup hook...");
	if(!SetupHook())
	{
		DPRINT("[NIGHTMARE DRV] Can't setup communication hook :(");
		return STATUS_ACCESS_DENIED;
	}
	
	DPRINT("[NIGHTMARE DRV] Setup callbacks...");
	PsSetLoadImageNotifyRoutine(ImageLoadCallback);
	PsSetCreateProcessNotifyRoutine(CreateProcessCallback, FALSE);

#ifndef DBG
	//EnableCallback();
#endif
	DPRINT("[NIGHTMARE DRV] Driver Loaded!");
	return STATUS_SUCCESS;

}


NTSTATUS DriverEP(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	return DriverEntry(DriverObject, RegistryPath);
}

