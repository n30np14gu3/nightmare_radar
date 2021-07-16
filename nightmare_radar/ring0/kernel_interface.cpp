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
	m_hDriver = LI_FN(CreateFileA)(ENCRYPT_STR_A(DRIVER_NAME),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr);

	NoErrors = false;
	m_dwErrorCode = 0;
	m_dwProcessId = 0;
	m_hProcess = INVALID_HANDLE_VALUE;
	Modules = new CSGoModules();
	if (m_hDriver == INVALID_HANDLE_VALUE)
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
	LI_FN(CloseHandle).cached()(m_hDriver);
	LI_FN(CloseHandle).cached()(m_hProcess);
	delete Modules;
}


bool KernelInterface::GetModules()
{
	NoErrors = false;

	if (m_hDriver == INVALID_HANDLE_VALUE)
		return false;


	KERNEL_GET_CLIENT_DLL req{ 0, 0, -1 };
	BOOL result = LI_FN(DeviceIoControl).cached()(m_hDriver, IO_GET_CLIENT_DLL, &req, sizeof(req), &req, sizeof(req), nullptr, nullptr);
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
	HANDLE hSnapshot = LI_FN(CreateToolhelp32Snapshot).cached()(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);
	do
		if (!strcmp(_bstr_t(entry.szExeFile), ENCRYPT_STR_A(GAME_NAME))) {
			m_dwProcessId = entry.th32ProcessID;
			m_hProcess = LI_FN(OpenProcess).cached()(SYNCHRONIZE, false, m_dwProcessId);
			LI_FN(CloseHandle).cached()(hSnapshot);
		}
	while (LI_FN(Process32Next).cached()(hSnapshot, &entry));
	if (m_dwProcessId == 0)
		return false;

	KERNEL_INIT_DATA_REQUEST req{ m_dwProcessId, LI_FN(GetCurrentProcessId).cached()(), -1 };

	if (m_hDriver == INVALID_HANDLE_VALUE)
		return false;

	const BOOL result = LI_FN(DeviceIoControl).cached()(m_hDriver, IO_INIT_CHEAT_DATA, &req, sizeof(req), &req, sizeof(req), nullptr, nullptr);
	if (!result)
	{
		m_dwErrorCode = LI_FN(GetLastError).cached()();
		NoErrors = false;
		return false;
	}

	if (!NT_SUCCESS(req.Result))
	{
		m_dwErrorCode = req.Result;
		NoErrors = false;
		return false;
	}

	return true;
}
