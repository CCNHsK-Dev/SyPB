//
// h_export.cpp
//
// Exports the Functions, binds original functions
// to ours, loads & initializes some custom stuff
//

#include "bot.h"

// Holds engine functionality callbacks
enginefuncs_t   g_engfuncs;
globalvars_t   *gpGlobals;

#ifdef _WIN32
HINSTANCE       h_Library = NULL;
#else
void           *h_Library = NULL;
#endif

GETENTITYAPI        other_GetEntityAPI = NULL;
GETNEWDLLFUNCTIONS  other_GetNewDLLFunctions = NULL;
GIVEFNPTRSTODLL     other_GiveFnptrsToDll = NULL;

#ifdef _WIN32

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
   if (fdwReason == DLL_PROCESS_DETACH)
   {
      FreeAllTheStuff();
      FreeLang();

      if (h_Library)
         FreeLibrary(h_Library);

      // free our table of exported symbols
      if (!g_bIsMMPlugin)
         FreeNameFuncGlobals();
   }
   return TRUE;
}

#else

void _fini(void)
{
   FreeAllTheStuff();
   FreeLang();
}

#endif

// If we're using MS compiler, we need to specify the export parameter...
// Jozef Wagner - START
#if _MSC_VER > 1000
#pragma comment(linker, "/EXPORT:GiveFnptrsToDll=_GiveFnptrsToDll@8,@1")
#pragma comment(linker, "/SECTION:.data,RW")
#endif
// Jozef Wagner - END

// Receive engine function table from engine.
// This appears to be the first DLL routine called by the engine, so we do some setup operations here.
void WINAPI GiveFnptrsToDll( enginefuncs_t* pengfuncsFromEngine, globalvars_t *pGlobals )
{
   // get the engine functions from the engine...
   memcpy(&g_engfuncs, pengfuncsFromEngine, sizeof(enginefuncs_t));
   gpGlobals = pGlobals;

   if (g_bIsMMPlugin)
      return;

   char szGameDLLName[256];
   GetGameDirectory(szGameDLLName);

   // load the actual Counter-Strike Game DLL
#ifdef _WIN32
   strcat(szGameDLLName, "\\dlls");
   mkdir(szGameDLLName);
   strcat(szGameDLLName, "\\mp.dll");

   h_Library = LoadLibrary(szGameDLLName);

   if (!h_Library)
   {
      // try to extract the game DLL out of the Steam cache
      int size;
      unsigned char *filebuf = LOAD_FILE_FOR_ME("dlls/mp.dll", &size);

      if (filebuf)
      {
         FILE *fp = fopen(szGameDLLName, "wb");
         if (fp)
         {
            // dump the file and close it
            fwrite(filebuf, size, 1, fp);
            fclose(fp);
         }
         FREE_FILE(filebuf);
      }

      h_Library = LoadLibrary(szGameDLLName);
   }
#else
   strcat(szGameDLLName, "/dlls/cs_i386.so");
   h_Library = dlopen(szGameDLLName, RTLD_NOW);
#endif

   if (!h_Library)
      TerminateOnError("Fail to load game DLL!");

   other_GetEntityAPI = (GETENTITYAPI)GetProcAddress(h_Library, "GetEntityAPI");
   other_GetNewDLLFunctions = (GETNEWDLLFUNCTIONS)GetProcAddress(h_Library, "GetNewDLLFunctions");
   other_GiveFnptrsToDll = (GIVEFNPTRSTODLL)GetProcAddress(h_Library, "GiveFnptrsToDll");

   GetEngineFunctions(pengfuncsFromEngine, NULL);

#ifdef _WIN32
   LoadSymbols(szGameDLLName);  // Load exported symbol table
   pengfuncsFromEngine->pfnFunctionFromName = FunctionFromName;
   pengfuncsFromEngine->pfnNameForFunction = NameForFunction;
#endif

   // give the engine functions to the other DLL...
   (*other_GiveFnptrsToDll)(pengfuncsFromEngine, pGlobals);
}

