#include "main.h"

#include "extdll.h"

// AMXX API
void OnPluginsLoaded()
{
	HMODULE dll = GetModuleHandle("sypb.dll");
	if(!dll)
	{
		LogToFile("Would not find the sypb.dll");
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

int LogToFile(char *szLogText, ...)
{
	FILE *fp;

	if (!(fp = fopen("spyb_amxx.txt", "a")))
		return 0;

	va_list vArgptr;
	char szText[1024];

	va_start(vArgptr, szLogText);
	vsprintf(szText, szLogText, vArgptr);
	va_end(vArgptr);

	fprintf(fp, " %s\n", szText);
	fclose(fp);
	return 1;
}
