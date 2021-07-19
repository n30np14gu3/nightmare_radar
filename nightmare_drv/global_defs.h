#pragma once
#define PROCESS_QUERY_LIMITED_INFORMATION      0x1000

#define DRIVER_NAME			L"sdfsdf22900"

#define DEVICE_NAME			(L"\\Device\\" DRIVER_NAME)
#define SYMBOL_NAME			(L"\\DosDevices\\" DRIVER_NAME)

#define CLIENT_DLL			ENCRYPT_STR_W(L"\\Counter-Strike Global Offensive\\csgo\\bin\\client.dll")
#define ENGINE_DLL			ENCRYPT_STR_W(L"\\Counter-Strike Global Offensive\\bin\\engine.dll")

#define CSGO_EXE			ENCRYPT_STR_A("csgo.exe")

#define CHEAT_NAME			ENCRYPT_STR_A("nightmare_radar.exe")


#define COPY_MEMORY_ROUTINE	(L"MmCopyVirtualMemory")


