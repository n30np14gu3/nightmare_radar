#include "globals.h"
#include "CreateProcessCallback.h"

extern CHAR* PsGetProcessImageFileName(IN PEPROCESS Process);

void CreateProcessCallback(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
{
	UNREFERENCED_PARAMETER(ParentId);
	if (!Create)
	{
		//Game closed
		if(ProcessId == GAME_PROCESS)
		{
			GAME_PROCESS = 0;
			PEGAME_PROCESS = 0;
			DRIVER_INITED = FALSE;
			DPRINT("[NIGHTMARE DRV] CS GO closed!");
			return;
		}

		//Cheat closed
		if(ProcessId == PROTECTED_PROCESS)
		{
			PROTECTED_PROCESS = 0;
			PEPROTECTED_PROCESS = 0;
			DRIVER_INITED = FALSE;
			DPRINT("[NIGHTMARE DRV] Cheat closed!");
		}
	}
}
