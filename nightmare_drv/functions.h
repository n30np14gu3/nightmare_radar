#pragma once
NTSTATUS UnsupportedDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);

//Driver unloading
void UnloadDriver(PDRIVER_OBJECT pDriverObject);

