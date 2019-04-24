#include "main.h"
#include "extdll.h"

#include <../../version.h>

bool sypbLog = false;

// AMXX API
void OnPluginsLoaded()
{
	sypbLog = false;

	SyPBDataLoad();
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