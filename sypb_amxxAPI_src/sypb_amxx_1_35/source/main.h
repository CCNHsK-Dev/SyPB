
#ifndef MAIN_H
#define MAIN_H

#include <windows.h>
#include <sys/types.h>	
#include <string.h>
#include <malloc.h>
#include "amxxmodule.h"

// API calling
extern AMX_NATIVE_INFO sypb_natives[];

void think(HMODULE dll);

// Error Log
int LogToFile(char *szLogText, ...);

#endif // MAIN_H