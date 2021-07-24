#pragma once
#pragma once
#include "../SDK/lazy_importer.hpp"
#include "../../nightmare_drv/driver_io.h"

struct CSGoModules
{
	DWORD32 bClient;
	DWORD32 bEngine;
};


class KernelInterface
{
public:
	BOOLEAN NoErrors;
	CSGoModules* Modules;

	KernelInterface();
	virtual bool Attach();
	virtual bool GetModules();
	virtual DWORD GetErrorCode() const;
	virtual ~KernelInterface();

	template <typename T>
	_inline void Read32(DWORD32 address, T* result)
	{
		NoErrors = false;

		if (m_hDriver == INVALID_HANDLE_VALUE)
			return;

		KERNEL_READ_REQUEST32 req;
		req.Size = sizeof(T);
		req.Address = address;
		req.Response = (DWORD64)result;
		req.result = -1;

		BOOL rsp = LI_FN(DeviceIoControl).cached()(m_hDriver, IO_READ_PROCESS_MEMORY_32, &req, sizeof(req), &req, sizeof(req), nullptr, nullptr);
		if (!rsp)
		{
			m_dwErrorCode = LI_FN(GetLastError).cached()();
		}
		else
		{
			if (!NT_SUCCESS(req.result))
			{
				m_dwErrorCode = req.result;
			}
		}

		NoErrors = true;
	}
private:
	HANDLE m_hDriver;
	DWORD m_dwErrorCode;
	DWORD m_dwProcessId;
};
