#include "globals.h"
#include "CreateProcessCallback.h"

extern CHAR* PsGetProcessImageFileName(IN PEPROCESS Process);

void CreateProcessCallback(HANDLE ParentId, HANDLE ProcessId, BOOLEAN Create)
{
	UNREFERENCED_PARAMETER(ParentId);
	if (!Create)
	{
		//Game or client closed
		if (ProcessId == PROTECTED_PROCESS || ProcessId == GAME_PROCESS)
		{
			PROTECTED_PROCESS = 0;
			GAME_PROCESS = 0;
			PEPROTECTED_PROCESS = 0;
			PEGAME_PROCESS = 0;
			DRIVER_INITED = FALSE;
			DPRINT("[NIGHTMARE DRV] Closing!");
		}
	}
	else
	{
		PEPROCESS pId;
		if(NT_SUCCESS(PsLookupProcessByProcessId(ProcessId, &pId)))
		{
			if(strstr(PsGetProcessImageFileName(pId), CHEAT_NAME))
			{
				PROTECTED_PROCESS = ProcessId;
				PsLookupProcessByProcessId(ProcessId, &PEPROTECTED_PROCESS);
				DRIVER_INITED = PROTECTED_PROCESS && GAME_PROCESS;
				DPRINT("[NIGHTMARE DRV] CHEAT STARTED=0x%X", PROTECTED_PROCESS);
				return;
			}

			if(strstr(PsGetProcessImageFileName(pId), CSGO_EXE))
			{
				GAME_PROCESS = ProcessId;
				PsLookupProcessByProcessId(ProcessId, &PEGAME_PROCESS);
				DRIVER_INITED = PROTECTED_PROCESS && GAME_PROCESS;
				DPRINT("[NIGHTMARE DRV] CSGO STARTED=0x%X", PROTECTED_PROCESS);
			}

			
		}
	}
}
