#pragma comment (lib,"urlmon.lib")

#include <Windows.h>
#include <SubAuth.h>
#include <comdef.h>

#include <ctime>
#include <iostream>


#include "SDK/globals.h"
#include "SDK/lazy_importer.hpp"
#include "ring0/kernel_interface.h"
#include "render/render.h"
#include <VMProtectSDK.h>

FILE* CON_OUT;
FILE* CON_IN;

std::string WINDOW_TITLE;

//Offsets
DWORD32 dwLocalPlayer;
DWORD32 dwEntityList;
DWORD32 dwViewMatrix;
DWORD32 dwClientState;


//Netvars
DWORD32 m_iHealth;
DWORD32 m_bDormant;
DWORD32 m_iTeamNum;
DWORD32 m_vecOrigin;
DWORD32 m_vecViewAngles;


//Settings
DWORD32 win_pos_x;
DWORD32 win_pos_y;

DWORD32 win_height;
DWORD32 win_width;

DWORD32 radar_zoom;
DWORD32 radar_point_size;

DWORD32 display_index;


void setup_console()
{
	LI_FN(srand)(static_cast<unsigned>(GetTickCount64()));
	LI_FN(AllocConsole)();

	WINDOW_TITLE = "";
	for (size_t i = 0; i <= 32; i++)
		WINDOW_TITLE += static_cast<char>(rand() % ('Z' - 'A') + 'A');


	LI_FN(SetConsoleTitleA)(WINDOW_TITLE.c_str());

	freopen_s(&CON_OUT, ENCRYPT_STR_A("CONOUT$"), "w", stdout);
	freopen_s(&CON_IN, ENCRYPT_STR_A("CONIN$"), "r", stdin);
}

void close_console()
{
	LI_FN(Sleep)(3000);
	fclose(CON_OUT);
	fclose(CON_IN);
	LI_FN(FreeConsole)();
	PostMessageA(LI_FN(GetConsoleWindow)(), WM_CLOSE, 0, 0);
}

bool file_exists(const char* file)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE handle = LI_FN(FindFirstFileA)(file, &FindFileData);
	bool found = handle != INVALID_HANDLE_VALUE;
	if (found)
		LI_FN(FindClose)(handle);
	return found;
}


bool update_offsets()
{
	const char* file_name = ENCRYPT_STR_A(".\\~tmp.ini");
	const char* offsets_section_name = ENCRYPT_STR_A("signatures");
	const char* netvars_section_name = ENCRYPT_STR_A("netvars");

	URLDownloadToFileA(nullptr, ENCRYPT_STR_A(OFFSETS_URL), file_name, 0, nullptr);
	LI_FN(SetFileAttributesA)(file_name, GetFileAttributesA(file_name) | FILE_ATTRIBUTE_HIDDEN);


	//Load offsets
	dwLocalPlayer = LI_FN(GetPrivateProfileIntA)(offsets_section_name, "dwLocalPlayer", 0, file_name);
	dwEntityList = LI_FN(GetPrivateProfileIntA)(offsets_section_name, "dwEntityList", 0, file_name);
	dwViewMatrix = LI_FN(GetPrivateProfileIntA)(offsets_section_name, "dwViewMatrix", 0, file_name);
	dwClientState = LI_FN(GetPrivateProfileIntA)(offsets_section_name, "dwClientState", 0, file_name);
	m_bDormant = LI_FN(GetPrivateProfileIntA)(offsets_section_name, "m_bDormant", 0, file_name);
	m_vecViewAngles = LI_FN(GetPrivateProfileIntA)(offsets_section_name, "dwClientState_ViewAngles", 0, file_name);

	//Load netvars
	m_iHealth = LI_FN(GetPrivateProfileIntA)(netvars_section_name, "m_iHealth", 0, file_name);
	m_iTeamNum = LI_FN(GetPrivateProfileIntA)(netvars_section_name, "m_iTeamNum", 0, file_name);
	m_vecOrigin = LI_FN(GetPrivateProfileIntA)(netvars_section_name, "m_vecOrigin", 0, file_name);


	return LI_FN(DeleteFileA)(ENCRYPT_STR_A(".\\~tmp.ini"));

}

bool load_settings()
{
	const char* file_name = ENCRYPT_STR_A(".\\config.ini");
	const char* config_section_name = ENCRYPT_STR_A("main_settings");

	if (!file_exists(file_name))
		return false;

	//load settings
	win_pos_x = LI_FN(GetPrivateProfileIntA)(config_section_name, "win_pos_x", 0, file_name);
	win_pos_y = LI_FN(GetPrivateProfileIntA)(config_section_name, "win_pos_y", 0, file_name);

	win_height = LI_FN(GetPrivateProfileIntA)(config_section_name, "win_height", 0, file_name);
	win_width = LI_FN(GetPrivateProfileIntA)(config_section_name, "win_width", 0, file_name);

	display_index = LI_FN(GetPrivateProfileIntA)(config_section_name, "display_index", 0, file_name);

	radar_zoom = LI_FN(GetPrivateProfileIntA)(config_section_name, "radar_zoom", 0, file_name);
	radar_point_size = LI_FN(GetPrivateProfileIntA)(config_section_name, "radar_point_size", 0, file_name);

	return true;
}


KernelInterface* ring0;

INT WINAPI WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	INT       nShowCmd
)
{
	VM_START("#WinMain_prepare");
	setup_console();
	printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] created by @shockbyte\n"));
	printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] created for @biawit\n"));
	printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] updating offsets...\n"));
	if (!update_offsets())
	{
		printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] can't update offsets! Closing...\n"));
		close_console();
		return 0;
	}
	printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] loading settings...\n"));
	if (!load_settings())
	{
		printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] can't load settings! Closing...\n"));
		close_console();
		return 0;
	}
	VM_END;

	ring0 = new KernelInterface();

	VM_START("#WinMain_driver_loading");
	printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] loading driver...\n"));
	if (!ring0->NoErrors)
	{
		printf(ENCRYPT_STR_A("[NIGHTMARE RADAR] can't load driver 0x%X\n"), ring0->GetErrorCode());
		close_console();
		return 0;
	}

	printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] driver loaded!\n"));
	printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] getting game info...\n"));
	while (!ring0->Attach()) {}
	while (!ring0->GetModules()) {}
	printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] setup is complete!\n"));
	printf_s(ENCRYPT_STR_A("[NIGHTMARE RADAR] starting render...\n"));
	close_console();
	VM_END;

	StartRender(WINDOW_TITLE.c_str(), ring0, hInstance);

	return 0;
}
