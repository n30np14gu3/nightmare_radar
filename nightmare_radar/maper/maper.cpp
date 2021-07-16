#include <stdio.h>
#include <Windows.h>
#include "maper.h"
#include "../SDK/lazy_importer.hpp"
#include <VMProtectSDK.h>

int map_image(image_data* buffer, unsigned int size) {
	void* (*NtConvertBetweenAuxiliaryCounterAndPerformanceCounter)(void*, void*, void*, void*);
	*reinterpret_cast<void**>(&NtConvertBetweenAuxiliaryCounterAndPerformanceCounter) = 
	LI_FN(GetProcAddress)(LI_FN(GetModuleHandleA)(ENCRYPT_STR_A("ntdll.dll")),
			ENCRYPT_STR_A("NtConvertBetweenAuxiliaryCounterAndPerformanceCounter")
		);

	if (!NtConvertBetweenAuxiliaryCounterAndPerformanceCounter) {
		printf_s(ENCRYPT_STR_A("NtConvertBetweenAuxiliaryCounterAndPerformanceCounter not found\n"));
		return 1;
	}

	unsigned short magic = *reinterpret_cast<unsigned short*>(buffer->buffer);

	unsigned int status = 0;
	NtConvertBetweenAuxiliaryCounterAndPerformanceCounter(0, &buffer, &status, 0);

	if (*reinterpret_cast<unsigned short*>(buffer->buffer) == magic) {
		printf(ENCRYPT_STR_A("[NIGHTMARE RADAR] failed to communicate with the mapper\n"));
		return 1;
	}

	if (buffer->buffer[0]) {
		printf(ENCRYPT_STR_A("[NIGHTMARE RADAR] manual mapping failed:\n\t%s\n"), buffer->buffer);
		return 1;
	}

	printf(ENCRYPT_STR_A("[NIGHTMARE RADAR] DriverEntry returned 0x%X\n"), status);

	return 0;
}
