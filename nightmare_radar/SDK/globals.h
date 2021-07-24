#pragma once

//Driver & Game Name
/**
 * \brief Driver device Name
 */
#define DRIVER_NAME						"\\\\.\\bwf_ac_1"

 /**
 * \brief Game for hacking
 */
#define GAME_NAME						"csgo.exe"

#define OFFSETS_URL						"https://raw.githubusercontent.com/frk1/hazedumper/master/csgo.toml"

 //Offsets
extern DWORD32 dwLocalPlayer;
extern DWORD32 dwEntityList;
extern DWORD32 dwViewMatrix;
extern DWORD32 dwClientState;

//Netvars
extern DWORD32 m_iHealth;
extern DWORD32 m_bDormant;
extern DWORD32 m_iTeamNum;
extern DWORD32 m_vecOrigin;
extern DWORD32 m_vecViewAngles;

//Settings
extern DWORD32 win_pos_x;
extern DWORD32 win_pos_y;

extern DWORD32 win_height;
extern DWORD32 win_width;

extern DWORD32 radar_zoom;
extern DWORD32 radar_point_size;


extern DWORD32 display_index;
