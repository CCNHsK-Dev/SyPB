
#ifndef MAIN_H
#define MAIN_H

#include <windows.h>
#include <sys/types.h>	
#include <string.h>
#include <malloc.h>
#include "amxxmodule.h"

// API calling
extern AMX_NATIVE_INFO sypb_natives[];

void SyPBDataLoad(void);

void ErrorWindows(const char *text);

// Error Log
int LogToFile(const char *szLogText, ...);

#endif // MAIN_H