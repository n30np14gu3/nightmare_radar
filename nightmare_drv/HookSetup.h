#pragma once
extern INT64(*HalDispatchOriginal)(PVOID, PVOID);

BOOLEAN SetupHook();

INT64 HalDispatchHook(void* IoData, PINT64 outStatus);

BOOLEAN SafeCopy(PVOID dest, PVOID src, SIZE_T size);
BOOLEAN MemCopyWP(PVOID dest, PVOID src, ULONG length);
BOOLEAN ReadFromUm(PVOID dest, PVOID src, SIZE_T size, PEPROCESS umProcess);
BOOLEAN WriteToUm(PVOID dest, PVOID src, SIZE_T size, PEPROCESS umProcess);
