#include "globals.h"
#include "global_defs.h"
#include "functions.h"

#include "ImageLoadCallback.h"
#include "CreateProcessCallback.h"

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


NTSTATUS NTAPI IoCreateDriver(_In_opt_ PUNICODE_STRING 	DriverName,
	_In_ PDRIVER_INITIALIZE 	InitializationFunction
);


NTSTATUS DriverInit(
	PDRIVER_OBJECT DriverObject,
	PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);

	DPRINT("[NIGHTMARE DRV] Loading...");

	DriverObject->DriverUnload = UnloadDriver;

	for (ULONG t = 0; t < IRP_MJ_MAXIMUM_FUNCTION; t++)
		DriverObject->MajorFunction[t] = UnsupportedDispatch;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCall;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = CloseCall;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoControl;
	
	DPRINT("[NIGHTMARE DRV] Setup callbacks...");
	PsSetLoadImageNotifyRoutine(ImageLoadCallback);
	PsSetCreateProcessNotifyRoutine(CreateProcessCallback, FALSE);


	DPRINT("[NIGHTMARE DRV] Creating device...");

	RtlSecureZeroMemory((PVOID)&DeviceName, sizeof(DeviceName));
	RtlInitUnicodeString(&DeviceName, DEVICE_NAME);
	RtlSecureZeroMemory((PVOID)&DosName, sizeof(DosName));
	RtlInitUnicodeString(&DosName, SYMBOL_NAME);

	NTSTATUS status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE,
		&pDeviceObj);
	IoCreateSymbolicLink(&DosName, &DeviceName);

	if (!NT_SUCCESS(status))
	{
		DPRINT("[NIGHTMARE DRV] Creating device Error: Code 0x%X", status);
		return status;
	}

	if (pDeviceObj != NULL)
	{
		pDeviceObj->Flags |= DO_DIRECT_IO;
		pDeviceObj->Flags &= ~DO_DEVICE_INITIALIZING;
	}
	
	DPRINT("[NIGHTMARE DRV] Getting routines...");
	RtlSecureZeroMemory((PVOID)&MemCopyRoutineName, sizeof(MemCopyRoutineName));
	RtlInitUnicodeString(&MemCopyRoutineName, COPY_MEMORY_ROUTINE);
	MemCopy = (MmCopyVirtualMemoryFn)MmGetSystemRoutineAddress(&MemCopyRoutineName);
	if (MemCopy == NULL)
	{
		DPRINT("[NIGHTMARE DRV] Can't find %s Routine!", COPY_MEMORY_ROUTINE);
		return STATUS_NOT_FOUND;
	}
	
	DPRINT("[NIGHTMARE DRV] Driver Loaded!");
	return STATUS_SUCCESS;
}


NTSTATUS NTAPI DriverEntry(PDRIVER_OBJECT driver, PUNICODE_STRING registryPath)
{
	UNREFERENCED_PARAMETER(driver);
	UNREFERENCED_PARAMETER(registryPath);
	
	UNICODE_STRING drv_name;
	RtlSecureZeroMemory((PVOID)&drv_name, sizeof(UNICODE_STRING));
	RtlInitUnicodeString(&drv_name, DRIVER_NAME2);
	return IoCreateDriver(&drv_name, &DriverInit);
}
