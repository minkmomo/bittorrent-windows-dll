// dllmain.cpp : DLL 응용 프로그램의 진입점을 정의합니다.
#include "bittorrent.h"
#include <windows.h>

#ifdef _DEBUG
#pragma comment (lib, "bittorrent_d.lib")
#else
#pragma comment (lib, "bittorrent.lib")
#endif

#pragma comment (lib, "WS2_32.lib")
#pragma comment (lib, "4758cca.lib")
#pragma comment (lib, "aep.lib")
#pragma comment (lib, "atalla.lib")
#pragma comment (lib, "capi.lib")
#pragma comment (lib, "chil.lib")
#pragma comment (lib, "cswift.lib")
#pragma comment (lib, "gost.lib")
#pragma comment (lib, "libeay32.lib")
#pragma comment (lib, "nuron.lib")
#pragma comment (lib, "padlock.lib")
#pragma comment (lib, "ssleay32.lib")
#pragma comment (lib, "sureware.lib")
#pragma comment (lib, "ubsec.lib")

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

