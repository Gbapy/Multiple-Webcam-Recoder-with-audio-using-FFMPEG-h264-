// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "WR.h"

BOOL APIENTRY DllMain(HINSTANCE hinst, DWORD reason, LPVOID unused1)
{
	if (reason == DLL_PROCESS_ATTACH) {
		dll_inst = hinst;
	}
	(void)unused1;
	return true;
}

