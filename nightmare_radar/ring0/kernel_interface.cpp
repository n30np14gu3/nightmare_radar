#include <Windows.h>
#include <SubAuth.h>
#include <TlHelp32.h>
#include <comdef.h>

#include <string>

#include "../SDK/globals.h"
#include <VMProtectSDK.h>
#include "kernel_interface.h"


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
	Modules = new CSGoModules();
	if (m_hDriver == INVALID_HANDLE_VALUE)
	{
		m_dwErrorCode = LI_FN(GetLastError)();
		return;

	}
	NoErrors = true;
	VM_END;
}

bool KernelInterface::Attach()
{
	VM_START("KernelInterface::Attach");
	HANDLE hSnapshot = LI_FN(CreateToolhelp32Snapshot).cached()(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(entry);
	do
		if (!strcmp(_bstr_t(entry.szExeFile), ENCRYPT_STR_A(GAME_NAME))) {
			m_dwProcessId = entry.th32ProcessID;
			//m_hProcess = LI_FN(OpenProcess).cached()(SYNCHRONIZE, false, m_dwProcessId);
			LI_FN(CloseHandle).cached()(hSnapshot);
		}
	while (LI_FN(Process32Next).cached()(hSnapshot, &entry));
	if (m_dwProcessId == 0)
		return false;

	KERNEL_SEND_PROCESSES req{
		(HANDLE)m_dwProcessId,
		(HANDLE)LI_FN(GetCurrentProcessId).cached()(),
		-1 };

	if (m_hDriver == INVALID_HANDLE_VALUE)
		return false;

	const BOOL result = LI_FN(DeviceIoControl).cached()(m_hDriver, IO_SETUP, &req, sizeof(req), &req, sizeof(req), nullptr, nullptr);
	if (!result)
	{
		m_dwErrorCode = LI_FN(GetLastError).cached()();
		NoErrors = false;
		return false;
	}

	if (!NT_SUCCESS(req.result))
	{
		m_dwErrorCode = req.result;
		NoErrors = false;
		return false;
	}
	VM_END;
	return true;

}

DWORD KernelInterface::GetErrorCode() const
{
	return m_dwErrorCode;
}

KernelInterface::~KernelInterface()
{
	LI_FN(CloseHandle).cached()(m_hDriver);
	delete Modules;
}


bool KernelInterface::GetModules()
{
	NoErrors = false;

	if (m_hDriver == INVALID_HANDLE_VALUE)
		return false;


	KERNEL_GET_CLIENT_DLL req{ 0, 0, -1 };
	BOOL result = LI_FN(DeviceIoControl).cached()(m_hDriver, IO_GET_MODULES, &req, sizeof(req), &req, sizeof(req), nullptr, nullptr);
	if (!result)
		return false;

	if (!NT_SUCCESS(req.result))
		return false;

	Modules->bClient = req.bClient;
	Modules->bEngine = req.bEngine;
	return true;
}
