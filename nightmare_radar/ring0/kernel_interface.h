#pragma once
#pragma once
#include "../SDK/lazy_importer.hpp"
#include "../../nightmare_drv/driver_io.h"

struct CSGoModules
{
	DWORD32 bClient;
	DWORD32 bEngine;
};

typedef void* (*fnNtHooked)(void*, void*, void*, void*);

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

		if (hook == nullptr)
			return;

		KERNEL_READ_REQUEST32 req;
		req.Size = sizeof(T);
		req.Address = address;
		req.Response = (DWORD64)result;
		req.Result = -1;

		KERNEL_HOOK_REQUEST hookReq{
			0x7331,
IO_READ_PROCESS_MEMORY_32,
&req,
sizeof(req),
		};
		UINT status = 0;

		const BOOL rsp = NT_SUCCESS(hook(nullptr, &hookReq, &status, nullptr));
		if (!rsp)
		{
			m_dwErrorCode = LI_FN(GetLastError).cached()();
		}
		else
		{
			if (!NT_SUCCESS(req.Result))
			{
				m_dwErrorCode = req.Result;
			}
		}

		NoErrors = true;

	}
private:
	DWORD m_dwErrorCode;
	const char* m_szFunction = nullptr;
	fnNtHooked hook = nullptr;
};
