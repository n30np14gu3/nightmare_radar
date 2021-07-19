#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#pragma comment(lib, "dwmapi.lib")

#include <d3d9.h>
#include <d3dx9.h>

#include <dwmapi.h>
#include <Windows.h>
#include <SubAuth.h>
#include <comdef.h>


#include <iostream>
#include <string>
#include <sstream>
#include <iostream>
#include <random>

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_dx9.h"
#include "../imgui/imgui_impl_win32.h"

#include "../SDK/lazy_importer.hpp"

#include <VMProtectSDK.h>
#include "../ring0/kernel_interface.h"
#include "../draw_utils/draw_utils.h"
#include "../SDK/globals.h"
#include "../SDK/CSGO/Entity.h"
#include "render.h"

using namespace std;

HWND OVERLAY_WINDOW;

INT SCREEN_WIDTH = 800;
INT SCREEN_HEIGHT = 600;

const MARGINS margin = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
RECT DESKTOP_RECT;
DWORD monitor_index;


DWORD32 pLocal;
DWORD32 localHp;
DWORD32 localTeam;
DWORD32 localCrosshairId;

DWORD32 pEntity;
DWORD32 entHp;
DWORD32 entTeam;
BOOL entDormant;
BOOL entSpotted;

LRESULT CALLBACK WinProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		DwmExtendFrameIntoClientArea(hWnd, &margin);
		break;

	case WM_DESTROY:
	case WM_CLOSE:
		PostQuitMessage(0);
		ExitProcess(0);

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

bool window_opened;

const ImGuiWindowFlags flags = 
ImGuiWindowFlags_NoTitleBar | 
ImGuiWindowFlags_NoSavedSettings |
ImGuiWindowFlags_NoCollapse |
ImGuiWindowFlags_NoResize |
ImGuiWindowFlags_NoMove;

enum EntityShape_t
{
	SHAPE_CIRCLE,
	SHAPE_TRIANGLE,
	SHAPE_TRIANGLE_UPSIDEDOWN
};

DWORD32 CLIENT_STATE;


Vector2D WorldToRadar(const Vector location, const Vector origin, const QAngle angles, int width, float scale = 16.f)
{
	float x_diff = location.x - origin.x;
	float y_diff = location.y - origin.y;

	int iRadarRadius = width;

	float flOffset = atanf(y_diff / x_diff);
	flOffset *= 180;
	flOffset /= M_PI;

	if ((x_diff < 0) && (y_diff >= 0))
		flOffset = 180 + flOffset;
	else if ((x_diff < 0) && (y_diff < 0))
		flOffset = 180 + flOffset;
	else if ((x_diff >= 0) && (y_diff < 0))
		flOffset = 360 + flOffset;

	y_diff = -1 * (sqrtf((x_diff * x_diff) + (y_diff * y_diff)));
	x_diff = 0;

	flOffset = angles.y - flOffset;

	flOffset *= M_PI;
	flOffset /= 180;

	float xnew_diff = x_diff * cosf(flOffset) - y_diff * sinf(flOffset);
	float ynew_diff = x_diff * sinf(flOffset) + y_diff * cosf(flOffset);

	xnew_diff /= scale;
	ynew_diff /= scale;

	xnew_diff = (iRadarRadius / 2) + (int)xnew_diff;
	ynew_diff = (iRadarRadius / 2) + (int)ynew_diff;

	// clamp x & y
	// FIXME: instead of using hardcoded "4" we should fix cliprect of the radar window
	if (xnew_diff > iRadarRadius)
		xnew_diff = iRadarRadius - 4;
	else if (xnew_diff < 4)
		xnew_diff = 4;

	if (ynew_diff > iRadarRadius)
		ynew_diff = iRadarRadius;
	else if (ynew_diff < 4)
		ynew_diff = 0;

	return Vector2D(xnew_diff, ynew_diff);
}

void draw_utils::hackProc(void* ptr)
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();


	ImGui::Begin("[DBG]", &window_opened, flags);
	{
		ImGui::SetWindowSize(ImVec2(static_cast<float>(win_width), static_cast<float>(win_height)));
		ImGui::SetWindowPos(ImVec2(static_cast<float>(win_pos_x), static_cast<float>(win_pos_y)));

		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 winpos = ImGui::GetWindowPos();
		ImVec2 winsize = ImGui::GetWindowSize();

		//Draw Lines
		draw_list->AddLine(ImVec2(winpos.x + winsize.x * 0.5f, winpos.y), ImVec2(winpos.x + winsize.x * 0.5f, winpos.y + winsize.y), ImColor(70, 70, 70, 255), 1.f);
		draw_list->AddLine(ImVec2(winpos.x, winpos.y + winsize.y * 0.5f), ImVec2(winpos.x + winsize.x, winpos.y + winsize.y * 0.5f), ImColor(70, 70, 70, 255), 1.f);

		draw_list->AddLine(ImVec2(winpos.x + winsize.x * 0.5f, winpos.y + winsize.y * 0.5f), ImVec2(winpos.x, winpos.y), ImColor(90, 90, 90, 255), 1.f);
		draw_list->AddLine(ImVec2(winpos.x + winsize.x * 0.5f, winpos.y + winsize.y * 0.5f), ImVec2(winpos.x + winsize.x, winpos.y), ImColor(90, 90, 90, 255), 1.f);

		//Draw LP
		draw_list->AddCircleFilled(ImVec2(winpos.x + winsize.x * 0.5f, winpos.y + winsize.y * 0.5f), static_cast<ImU32>(radar_point_size), ImColor(0, 0, 255, 255));

		float scale = static_cast<float>(radar_point_size);
	

		Entity localPlayer(ptr, CLIENT_STATE);
		//Remove localPlayer Health Check
		if(!localPlayer.isValid() /*|| !localPlayer.getHealth()*/)
			goto end_render;

		QAngle localplayer_angles = localPlayer.getViewAngles();
		Vector localPos = localPlayer.getOrigin();
		
		for(DWORD32 i = 1; i <= 64; i++)
		{
			EntityShape_t shape;

			Entity ent(ptr, CLIENT_STATE, i);
			if(!ent.isValid())
				continue;

			if(ent.isDormant() || !ent.getHealth())
				continue;

			if(ent.getTeam() == localPlayer.getTeam())
				continue;

			Vector playerPos = ent.getOrigin();
			Vector2D screenpos = WorldToRadar(playerPos, localPos, localplayer_angles, static_cast<int>(winsize.x), static_cast<float>(radar_zoom));

			if (playerPos.z + 64.0f < localPos.z)
				shape = SHAPE_TRIANGLE_UPSIDEDOWN;
			else if (playerPos.z - 64.0f > localPos.z)
				shape = SHAPE_TRIANGLE;
			else
				shape = SHAPE_CIRCLE;
			
			switch (shape)
			{
			case SHAPE_CIRCLE:
				draw_list->AddCircleFilled(ImVec2(winpos.x + screenpos.x, winpos.y + screenpos.y), scale, ImColor(255, 0, 0));
				break;
			case SHAPE_TRIANGLE:
				draw_list->AddTriangleFilled(ImVec2(winpos.x + screenpos.x + scale, winpos.y + screenpos.y + scale),
					ImVec2(winpos.x + screenpos.x - scale, winpos.y + screenpos.y + scale),
					ImVec2(winpos.x + screenpos.x, winpos.y + screenpos.y - scale),
					ImColor(255, 0, 0));
				break;
			case SHAPE_TRIANGLE_UPSIDEDOWN:
				draw_list->AddTriangleFilled(ImVec2(winpos.x + screenpos.x - scale, winpos.y + screenpos.y - scale),
					ImVec2(winpos.x + screenpos.x + scale, winpos.y + screenpos.y - scale),
					ImVec2(winpos.x + screenpos.x, winpos.y + screenpos.y + scale),
					ImColor(255, 0, 0));
				break;
			}
		}
		
	}
	end_render:
	ImGui::End();
	ImGui::EndFrame();

	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}


BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
	if (monitor_index == display_index)
	{
		DESKTOP_RECT = *lprcMonitor;
		SCREEN_WIDTH = DESKTOP_RECT.right - DESKTOP_RECT.left;
		SCREEN_HEIGHT = DESKTOP_RECT.bottom - DESKTOP_RECT.top;
		return false;
	}

	monitor_index++;
	
	return true;
}

void StartRender(
	const char* windowName,
	KernelInterface* ring0,
	HINSTANCE hInstance
)
{
	/*HWND desktop = LI_FN(GetDesktopWindow)();
	if (desktop != nullptr)
	{
		LI_FN(GetWindowRect)(desktop, &DESKTOP_RECT);
		SCREEN_WIDTH = DESKTOP_RECT.right - DESKTOP_RECT.left;
		SCREEN_HEIGHT = DESKTOP_RECT.bottom - DESKTOP_RECT.top;
	}
	else
	{
		LI_FN(MessageBoxA)(nullptr, ENCRYPT_STR_A("Can't find desktop!"), ENCRYPT_STR_A("ERROR"), MB_OK);
		LI_FN(ExitProcess)(0);
	}*/

	
	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, NULL);
	WNDCLASSA wc;
	ZeroMemory(&wc, sizeof(WNDCLASSA));

	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WinProc;
	wc.hInstance = hInstance;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = HBRUSH(RGB(0, 0, 0));
	wc.lpszClassName = windowName;

	RegisterClassA(&wc);

	OVERLAY_WINDOW = LI_FN(CreateWindowExA).cached()(0, windowName, windowName, WS_EX_TOPMOST | WS_POPUP, DESKTOP_RECT.left, DESKTOP_RECT.top, SCREEN_WIDTH, SCREEN_HEIGHT, nullptr, nullptr, hInstance, nullptr);
	LI_FN(SetWindowLongA)(OVERLAY_WINDOW, GWL_EXSTYLE, (int)GetWindowLong(OVERLAY_WINDOW, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT);
	LI_FN(SetLayeredWindowAttributes)(OVERLAY_WINDOW, RGB(0, 0, 0), 255, ULW_COLORKEY | LWA_ALPHA);
	LI_FN(SetWindowDisplayAffinity)(OVERLAY_WINDOW, WDA_MONITOR);
	LI_FN(SetWindowPos)(OVERLAY_WINDOW, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	LI_FN(ShowWindow)(OVERLAY_WINDOW, SW_SHOWDEFAULT);
	LI_FN(UpdateWindow)(OVERLAY_WINDOW);

	MSG msg = {};
	draw_utils::init_utils(OVERLAY_WINDOW, DESKTOP_RECT);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsLight();

	ImGui_ImplWin32_Init(OVERLAY_WINDOW);
	ImGui_ImplDX9_Init(draw_utils::m_dxDevice);

	ring0->Read32(ring0->Modules->bEngine + dwClientState, &CLIENT_STATE);
	
	while (msg.message != WM_QUIT)
	{
		if (LI_FN(PeekMessageA).cached()(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			LI_FN(TranslateMessage).cached()(&msg);
			LI_FN(DispatchMessageA).cached()(&msg);
			continue;
		}

		LI_FN(SetWindowPos).cached()(OVERLAY_WINDOW, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE); (OVERLAY_WINDOW, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

		draw_utils::render(ring0);
	}

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
