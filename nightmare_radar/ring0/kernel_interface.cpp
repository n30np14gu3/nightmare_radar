#include <Windows.h>
#include <SubAuth.h>
#include <TlHelp32.h>
#include <comdef.h>

#include <string>

#include "../SDK/globals.h"
#include <VMProtectSDK.h>
#include "kernel_interface.h"

typedef BOOLEAN(WINAPI* pRtlDosPathNameToNtPathName_U)(PCWSTR DosFileName, PUNICODE_STRING NtFileName, PWSTR* FilePart, PVOID RelativeName);
typedef void(WINAPI* pRtlFreeUnicodeString)(PUNICODE_STRING UnicodeString);

KernelInterface::KernelInterface()
{
	VM_START("KernelInterface::KernelInterface");
	NoErrors = false;
	m_dwErrorCode = 0;
	Modules = new CSGoModules();
	m_szFunction = ENCRYPT_STR_A("NtConvertBetweenAuxiliaryCounterAndPerformanceCounter");
	hook = (fnNtHooked)LI_FN(GetProcAddress)(LI_FN(GetModuleHandleA)(ENCRYPT_STR_A("ntdll.dll")), m_szFunction);
	if(!hook)
	{
		m_dwErrorCode = LI_FN(GetLastError)();
		return;
	}

	NoErrors = true;
	VM_END;
}

DWORD KernelInterface::GetErrorCode() const
{
	return m_dwErrorCode;
}

KernelInterface::~KernelInterface()
{
	delete Modules;
}


bool KernelInterface::GetModules()
{
	NoErrors = false;

	if (hook == nullptr)
		return false;


	KERNEL_GET_CLIENT_DLL req{ 0, 0, -1 };
	KERNEL_HOOK_REQUEST hookReq{
				0x7331,
		IO_GET_CLIENT_DLL,
		&req,
		sizeof(req),
	};
	UINT status = 0;
	
	const BOOL result = NT_SUCCESS(hook(nullptr, &hookReq, &status, nullptr));
	if (!result)
		return false;

	if (!NT_SUCCESS(req.result))
		return false;

	Modules->bClient = req.bClient;
	Modules->bEngine = req.bEngine;
	return true;
}

bool KernelInterface::Attach()
{
	return true;
}
