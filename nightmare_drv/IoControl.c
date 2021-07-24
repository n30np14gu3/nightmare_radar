#include "globals.h"

#define MM_TAG 'cf2'

NTSTATUS KeReadVirtualMemory32(PEPROCESS Process, DWORD32 SourceAddress, DWORD64 TargetAddress, SIZE_T Size, PSIZE_T ReadedBytes);

void GetModules(PIRP Irp);

#define WINDOWS_1803 17134
#define WINDOWS_1809 17763
#define WINDOWS_1903 18362
#define WINDOWS_1909 18363
#define WINDOWS_2004 19041
#define WINDOWS_20H2 19569
#define WINDOWS_21H1 20180

DWORD32 GetUserDirectoryTableBaseOffset()
{
	RTL_OSVERSIONINFOW ver = { 0 };
	RtlGetVersion(&ver);

	switch (ver.dwBuildNumber)
	{
	case WINDOWS_1803:
		return 0x0278;
	case WINDOWS_1809:
		return 0x0278;
	case WINDOWS_1903:
		return 0x0280;
	case WINDOWS_1909:
		return 0x0280;
	case WINDOWS_2004:
		return 0x0388;
	case WINDOWS_20H2:
		return 0x0388;
	case WINDOWS_21H1:
		return 0x0388;
	default:
		return 0x0388;
	}
}

NTSTATUS ReadPhysicalAddress(PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesRead)
{
	MM_COPY_ADDRESS AddrToRead = { 0 };
	AddrToRead.PhysicalAddress.QuadPart = (LONGLONG)TargetAddress;
	return MmCopyMemory(lpBuffer, AddrToRead, Size, MM_COPY_MEMORY_PHYSICAL, BytesRead);
}

//MmMapIoSpaceEx limit is page 4096 byte
NTSTATUS WritePhysicalAddress(PVOID TargetAddress, PVOID lpBuffer, SIZE_T Size, SIZE_T* BytesWritten)
{
	if (!TargetAddress)
		return STATUS_UNSUCCESSFUL;

	PHYSICAL_ADDRESS AddrToWrite = { 0 };
	AddrToWrite.QuadPart = (LONGLONG)TargetAddress;

	PVOID pmapped_mem = MmMapIoSpaceEx(AddrToWrite, Size, PAGE_READWRITE);

	if (!pmapped_mem)
		return STATUS_UNSUCCESSFUL;

	memcpy(pmapped_mem, lpBuffer, Size);

	*BytesWritten = Size;
	MmUnmapIoSpace(pmapped_mem, Size);
	return STATUS_SUCCESS;
}

#define PAGE_OFFSET_SIZE 12
static const UINT64 PMASK = (~0xfull << 8) & 0xfffffffffull;

ULONG_PTR GetProcessCr3(PEPROCESS pProcess)
{
	PUCHAR process = (PUCHAR)pProcess;
	ULONG_PTR process_dirbase = *(PULONG_PTR)(process + 0x28); //dirbase x64, 32bit is 0x18
	if (process_dirbase == 0)
	{
		DWORD64 UserDirOffset = GetUserDirectoryTableBaseOffset();
		ULONG_PTR process_userdirbase = *(PULONG_PTR)(process + UserDirOffset);
		return process_userdirbase;
	}
	return process_dirbase;
}

UINT64 TranslateLinearAddress(UINT64 directoryTableBase, UINT64 virtualAddress) {
	directoryTableBase &= ~0xf;

	UINT64 pageOffset = virtualAddress & ~(~0ul << PAGE_OFFSET_SIZE);
	UINT64 pte = ((virtualAddress >> 12) & (0x1ffll));
	UINT64 pt = ((virtualAddress >> 21) & (0x1ffll));
	UINT64 pd = ((virtualAddress >> 30) & (0x1ffll));
	UINT64 pdp = ((virtualAddress >> 39) & (0x1ffll));

	SIZE_T readsize = 0;
	UINT64 pdpe = 0;
	ReadPhysicalAddress((PVOID)(directoryTableBase + 8 * pdp), &pdpe, sizeof(pdpe), &readsize);
	if (~pdpe & 1)
		return 0;

	UINT64 pde = 0;
	ReadPhysicalAddress((PVOID)((pdpe & PMASK) + 8 * pd), &pde, sizeof(pde), &readsize);
	if (~pde & 1)
		return 0;

	/* 1GB large page, use pde's 12-34 bits */
	if (pde & 0x80)
		return (pde & (~0ull << 42 >> 12)) + (virtualAddress & ~(~0ull << 30));

	UINT64 pteAddr = 0;
	ReadPhysicalAddress((PVOID)((pde & PMASK) + 8 * pt), &pteAddr, sizeof(pteAddr), &readsize);
	if (~pteAddr & 1)
		return 0;

	/* 2MB large page */
	if (pteAddr & 0x80)
		return (pteAddr & PMASK) + (virtualAddress & ~(~0ull << 21));

	virtualAddress = 0;
	ReadPhysicalAddress((PVOID)((pteAddr & PMASK) + 8 * pte), &virtualAddress, sizeof(virtualAddress), &readsize);
	virtualAddress &= PMASK;

	if (!virtualAddress)
		return 0;

	return virtualAddress + pageOffset;
}


//
NTSTATUS ReadProcessMemory(int pid, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* read)
{
	PEPROCESS pProcess = NULL;
	if (pid == 0) return STATUS_UNSUCCESSFUL;

	NTSTATUS NtRet = PsLookupProcessByProcessId((HANDLE)pid, &pProcess);
	if (NtRet != STATUS_SUCCESS) return NtRet;

	ULONG_PTR process_dirbase = GetProcessCr3(pProcess);
	ObDereferenceObject(pProcess);

	SIZE_T CurOffset = 0;
	SIZE_T TotalSize = size;
	while (TotalSize)
	{

		UINT64 CurPhysAddr = TranslateLinearAddress(process_dirbase, (ULONG64)Address + CurOffset);
		if (!CurPhysAddr) return STATUS_UNSUCCESSFUL;

		ULONG64 ReadSize = min(PAGE_SIZE - (CurPhysAddr & 0xFFF), TotalSize);
		SIZE_T BytesRead = 0;
		NtRet = ReadPhysicalAddress((PVOID)CurPhysAddr, (PVOID)((ULONG64)AllocatedBuffer + CurOffset), ReadSize, &BytesRead);
		TotalSize -= BytesRead;
		CurOffset += BytesRead;
		if (NtRet != STATUS_SUCCESS) break;
		if (BytesRead == 0) break;
	}

	*read = CurOffset;
	return NtRet;
}

NTSTATUS WriteProcessMemory(int pid, PVOID Address, PVOID AllocatedBuffer, SIZE_T size, SIZE_T* written)
{
	PEPROCESS pProcess = NULL;
	if (pid == 0) return STATUS_UNSUCCESSFUL;

	NTSTATUS NtRet = PsLookupProcessByProcessId((HANDLE)pid, &pProcess);
	if (NtRet != STATUS_SUCCESS) return NtRet;

	ULONG_PTR process_dirbase = GetProcessCr3(pProcess);
	ObDereferenceObject(pProcess);

	SIZE_T CurOffset = 0;
	SIZE_T TotalSize = size;
	while (TotalSize)
	{
		UINT64 CurPhysAddr = TranslateLinearAddress(process_dirbase, (ULONG64)Address + CurOffset);
		if (!CurPhysAddr) return STATUS_UNSUCCESSFUL;

		ULONG64 WriteSize = min(PAGE_SIZE - (CurPhysAddr & 0xFFF), TotalSize);
		SIZE_T BytesWritten = 0;
		NtRet = WritePhysicalAddress((PVOID)CurPhysAddr, (PVOID)((ULONG64)AllocatedBuffer + CurOffset), WriteSize, &BytesWritten);
		TotalSize -= BytesWritten;
		CurOffset += BytesWritten;
		if (NtRet != STATUS_SUCCESS) break;
		if (BytesWritten == 0) break;
	}

	*written = CurOffset;
	return NtRet;
}

ULONG_PTR GetKernelDirBase()
{
	PUCHAR process = (PUCHAR)PsGetCurrentProcess();
	ULONG_PTR cr3 = *(PULONG_PTR)(process + 0x28); //dirbase x64, 32bit is 0x18
	return cr3;
}

NTSTATUS ReadVirtual(UINT64 dirbase, UINT64 address, UINT8* buffer, SIZE_T size, SIZE_T* read)
{
	UINT64 paddress = TranslateLinearAddress(dirbase, address);
	return ReadPhysicalAddress((PVOID)paddress, buffer, size, read);
}

NTSTATUS WriteVirtual(UINT64 dirbase, UINT64 address, UINT8* buffer, SIZE_T size, SIZE_T* written)
{
	UINT64 paddress = TranslateLinearAddress(dirbase, address);
	return WritePhysicalAddress((PVOID)paddress, buffer, size, written);
}


NTSTATUS KeReadVirtualMemory32(PEPROCESS Process, DWORD32 SourceAddress, DWORD64 TargetAddress, SIZE_T Size, PSIZE_T ReadedBytes)
{
	UNREFERENCED_PARAMETER(Process);
	
	if (!MemCopy)
		return STATUS_NOT_FOUND;

	PVOID tmpBuff = ExAllocatePoolWithTag(NonPagedPool, Size, MM_TAG);
	if (!tmpBuff)
		return STATUS_MEMORY_NOT_ALLOCATED;

	NTSTATUS result = ReadProcessMemory(
		(int)(DWORD64)GAME_PROCESS, 
		(PVOID)(DWORD64)SourceAddress, 
		tmpBuff,
		Size, 
		ReadedBytes);
	
	if (!NT_SUCCESS(result))
	{
		ExFreePoolWithTag(tmpBuff, MM_TAG);
		return STATUS_ACCESS_DENIED;
	}

	result = MemCopy(
		PsGetCurrentProcess(),
		tmpBuff,
		PEPROTECTED_PROCESS,
		(PVOID64)TargetAddress,
		Size,
		KernelMode,
		ReadedBytes
	);
	ExFreePoolWithTag(tmpBuff, MM_TAG);

	return result;
}

void GetModules(PIRP Irp)
{
	PKERNEL_GET_CLIENT_DLL pModules = Irp->AssociatedIrp.SystemBuffer;
	if (pModules != NULL)
	{
		if (CLIENT_DLL_BASE == 0 || ENGINE_DLL_BASE == 0)
			pModules->result = STATUS_ACCESS_DENIED;
		else
		{
			pModules->bClient = CLIENT_DLL_BASE;
			pModules->bEngine = ENGINE_DLL_BASE;
			pModules->result = STATUS_SUCCESS;
		}
	}
}

void InitDriver(PIRP Irp)
{
	PKERNEL_SEND_PROCESSES pData = Irp->AssociatedIrp.SystemBuffer;
	if(pData != NULL)
	{
		PROTECTED_PROCESS = pData->cheatId;
		GAME_PROCESS = pData->gameId;

		pData->result = PsLookupProcessByProcessId(PROTECTED_PROCESS, &PEPROTECTED_PROCESS);
		pData->result |= PsLookupProcessByProcessId(GAME_PROCESS, &PEGAME_PROCESS);

		DRIVER_INITED = NT_SUCCESS(pData->result);
	}
}

NTSTATUS IoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	ULONG bytesIO = 0;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);
	const ULONG controlCode = stack->Parameters.DeviceIoControl.IoControlCode;

	PKERNEL_READ_REQUEST32 pReadRequest32;
	SIZE_T rwBytes = 0;
	
	switch (controlCode)
	{
	case IO_READ_PROCESS_MEMORY_32:
		pReadRequest32 = (PKERNEL_READ_REQUEST32)Irp->AssociatedIrp.SystemBuffer;
		if (!DRIVER_INITED)
		{
			pReadRequest32->result = STATUS_ACCESS_DENIED;
			break;
		}
		if (pReadRequest32 != NULL)
		{
			pReadRequest32->result = KeReadVirtualMemory32(PEGAME_PROCESS, pReadRequest32->Address, (pReadRequest32->Response), pReadRequest32->Size, &rwBytes);
		}
		bytesIO = sizeof(KERNEL_READ_REQUEST32);
		break;

	case IO_GET_MODULES:
		GetModules(Irp);
		bytesIO = sizeof(KERNEL_GET_CLIENT_DLL);
		break;

	case IO_SETUP:
		InitDriver(Irp);
		bytesIO = sizeof(KERNEL_SEND_PROCESSES);
		break;
	default:
		bytesIO = 0;
		break;
	}

	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = bytesIO;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}
