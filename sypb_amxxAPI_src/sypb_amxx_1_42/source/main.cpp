#include "main.h"
#include "extdll.h"

#include "version.h"

bool sypbLog = false;

// AMXX API
void OnPluginsLoaded()
{
	sypbLog = false;

	HMODULE dll = GetModuleHandle("sypb.dll");
	if(!dll)
	{
		LogToFile("***************************");
		LogToFile("We cannot find sypb.dll, SyPB API cannot run");
		LogToFile("***************************");

		ErrorWindows("We cannot find sypb.dll, SyPB API cannot run"
			"\n\nExit the Game?");
		return;
	}

	think(dll);
}

void OnAmxxAttach()
{
	MF_AddNatives( sypb_natives );

}
void OnAmxxDetach()
{
	
}

void ErrorWindows(char *text)
{
	int button = ::MessageBox(NULL, text, "SyPB API Error", MB_YESNO | MB_TOPMOST | MB_ICONINFORMATION);
	if (button == IDYES)
		exit(1);
}