#include "main.h"
#include "extdll.h"

#include <../../version.h>


// AMXX API
void OnPluginsLoaded()
{
	SyPBDataLoad();
}

void OnAmxxAttach()
{
	MF_AddNatives( sypb_natives );

}
void OnAmxxDetach()
{
	
}

void ErrorWindows(const char *text)
{
	const int button = ::MessageBox(NULL, text, "SyPB API Error", MB_YESNO | MB_TOPMOST | MB_ICONINFORMATION);
	if (button == IDYES)
		exit(1);
}