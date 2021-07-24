#pragma once
NTSTATUS UnsupportedDispatch(PDEVICE_OBJECT DeviceObject, PIRP Irp);

//Driver unloading
void UnloadDriver(PDRIVER_OBJECT pDriverObject);

//IoControll functions
NTSTATUS CreateCall(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS CloseCall(PDEVICE_OBJECT DeviceObject, PIRP irp);
NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
