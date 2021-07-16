#include "globals.h"

#include "ImageLoadCallback.h"
#include "CreateProcessCallback.h"

void UnloadDriver(PDRIVER_OBJECT pDriverObject)
{
	IoDeleteSymbolicLink(&DosName);
	IoDeleteDevice(pDriverObject->DeviceObject);
	PsRemoveLoadImageNotifyRoutine(ImageLoadCallback);
	PsSetCreateProcessNotifyRoutine(CreateProcessCallback, TRUE);
	DPRINT("[NIGHTMARE DRV] Driver Unloaded!");
}
