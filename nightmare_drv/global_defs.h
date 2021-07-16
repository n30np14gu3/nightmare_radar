#pragma once
#define PROCESS_QUERY_LIMITED_INFORMATION      0x1000

#define DRIVER_NAME			L"acpiex_device"

#define DEVICE_NAME			(L"\\Device\\" DRIVER_NAME)
#define SYMBOL_NAME			(L"\\DosDevices\\" DRIVER_NAME)

#define CLIENT_DLL			(L"\\Counter-Strike Global Offensive\\csgo\\bin\\client.dll")
#define ENGINE_DLL			(L"\\Counter-Strike Global Offensive\\bin\\engine.dll")

#define COPY_MEMORY_ROUTINE	(L"MmCopyVirtualMemory")
