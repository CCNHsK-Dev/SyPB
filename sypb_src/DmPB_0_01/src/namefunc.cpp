/*
Copyright (c) Pierre-Marie Baty.
All rights reserved.

Redistribution and use in source and binary forms with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer. 

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution. 

Neither the name of this project nor the names of its contributors may be used
to endorse or promote products derived from this software without specific
prior written permission. 

THIS SOFTWARE IS PROVIDED BY PM "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

// RACC - AI development project for first-person shooter games derivated from Valve's Half-Life
// (http://www.racc-ai.com/)
//
// Rational Autonomous Cybernetic Commandos AI
//
// Since there is a known issue on Win32 platforms that prevents
// hook DLLs (such as our bot DLL) to be used in single player
// games (Training Room) because they don't export all the stuff
// they should, we may need to build our own array of exported
// symbols from the actual game DLL in order to use it as such
// if necessary.
//

#ifdef _WIN32

#include "bot.h"

// file-related structure definitions
typedef struct
{
   WORD e_magic; // magic number
   WORD e_cblp; // bytes on last page of file
   WORD e_cp; // pages in file
   WORD e_crlc; // relocations
   WORD e_cparhdr; // size of header in paragraphs
   WORD e_minalloc; // minimum extra paragraphs needed
   WORD e_maxalloc; // maximum extra paragraphs needed
   WORD e_ss; // initial (relative) SS value
   WORD e_sp; // initial SP value
   WORD e_csum; // checksum
   WORD e_ip; // initial IP value
   WORD e_cs; // initial (relative) CS value
   WORD e_lfarlc; // file address of relocation table
   WORD e_ovno; // overlay number
   WORD e_res[4]; // reserved words
   WORD e_oemid; // OEM identifier (for e_oeminfo)
   WORD e_oeminfo; // OEM information; e_oemid specific
   WORD e_res2[10]; // reserved words
   LONG e_lfanew; // file address of new exe header
} DOS_HEADER, *P_DOS_HEADER; // DOS .EXE header

typedef struct
{
   WORD Machine;
   WORD NumberOfSections;
   DWORD TimeDateStamp;
   DWORD PointerToSymbolTable;
   DWORD NumberOfSymbols;
   WORD SizeOfOptionalHeader;
   WORD Characteristics;
} PE_HEADER, *P_PE_HEADER;

typedef struct
{
   BYTE Name[8];
   union
   {
      DWORD PhysicalAddress;
      DWORD VirtualSize;
   } Misc;
   DWORD VirtualAddress;
   DWORD SizeOfRawData;
   DWORD PointerToRawData;
   DWORD PointerToRelocations;
   DWORD PointerToLinenumbers;
   WORD NumberOfRelocations;
   WORD NumberOfLinenumbers;
   DWORD Characteristics;
} SECTION_HEADER, *P_SECTION_HEADER;

typedef struct
{
   DWORD VirtualAddress;
   DWORD Size;
} DATA_DIRECTORY, *P_DATA_DIRECTORY;

typedef struct
{
   WORD Magic;
   BYTE MajorLinkerVersion;
   BYTE MinorLinkerVersion;
   DWORD SizeOfCode;
   DWORD SizeOfInitializedData;
   DWORD SizeOfUninitializedData;
   DWORD AddressOfEntryPoint;
   DWORD BaseOfCode;
   DWORD BaseOfData;
   DWORD ImageBase;
   DWORD SectionAlignment;
   DWORD FileAlignment;
   WORD MajorOperatingSystemVersion;
   WORD MinorOperatingSystemVersion;
   WORD MajorImageVersion;
   WORD MinorImageVersion;
   WORD MajorSubsystemVersion;
   WORD MinorSubsystemVersion;
   DWORD Win32VersionValue;
   DWORD SizeOfImage;
   DWORD SizeOfHeaders;
   DWORD CheckSum;
   WORD Subsystem;
   WORD DllCharacteristics;
   DWORD SizeOfStackReserve;
   DWORD SizeOfStackCommit;
   DWORD SizeOfHeapReserve;
   DWORD SizeOfHeapCommit;
   DWORD LoaderFlags;
   DWORD NumberOfRvaAndSizes;
   DATA_DIRECTORY DataDirectory[16];
} OPTIONAL_HEADER, *P_OPTIONAL_HEADER;

typedef struct
{
   DWORD Characteristics;
   DWORD TimeDateStamp;
   WORD MajorVersion;
   WORD MinorVersion;
   DWORD Name;
   DWORD Base;
   DWORD NumberOfFunctions;
   DWORD NumberOfNames;
   DWORD AddressOfFunctions; // RVA from base of image
   DWORD AddressOfNames; // RVA from base of image
   DWORD AddressOfNameOrdinals; // RVA from base of image
} EXPORT_DIRECTORY, *P_EXPORT_DIRECTORY;

WORD *p_Ordinals = NULL;
DWORD *p_Functions = NULL;
DWORD *p_Names = NULL;
char **p_FunctionNames = NULL;
int num_ordinals = 0;
unsigned long base_offset;

void FreeNameFuncGlobals(void)
{
   // free our table of exported symbols (only on Win32 platforms)
   if (p_Ordinals)
      free(p_Ordinals);
   p_Ordinals = NULL;

   if (p_Functions)
      free(p_Functions);
   p_Functions = NULL;

   if (p_Names)
      free(p_Names);
   p_Names = NULL;

   if (p_FunctionNames)
   {
      for (int index = 0; index < num_ordinals; index++)
      {
         if (p_FunctionNames[index])
            free(p_FunctionNames[index]);
         p_FunctionNames[index] = NULL;
      }
      free(p_FunctionNames);
   }
   p_FunctionNames = NULL;
}

void LoadSymbols(const char *filename)
{
   // the purpose of this function is to perfect the bot DLL interfacing. Having all the
   // MOD entities listed and linked to their proper function with LINK_ENTITY_TO_FUNC is
   // not enough, procs are missing, and that's the reason why most bot DLLs don't allow
   // to run single player games. This function loads the symbols in the game DLL by hand,
   // strips their MSVC-style case mangling, and builds an exports array which supercedes
   // the one the engine would get afterwards from the MOD DLL, which can't pass through
   // the bot DLL. This way we are sure that *nothing is missing* in the interfacing. Note
   // this is a fix for WIN32 systems only. But since UNIX systems only host dedicated
   // servers, there's no need to run single-player games on them.

   FILE *fp;
   DOS_HEADER dos_header;
   LONG nt_signature;
   PE_HEADER pe_header;
   SECTION_HEADER section_header;
   OPTIONAL_HEADER optional_header;
   LONG edata_offset;
   LONG edata_delta;
   EXPORT_DIRECTORY export_directory;
   LONG name_offset;
   LONG ordinal_offset;
   LONG function_offset;
   char function_name[256], ch;
   int i, j;
   void *game_GiveFnptrsToDll;

   // open MOD DLL file in binary read mode
   fp = fopen(filename, "rb"); // can't fail to do this, since we LoadLibrary()'ed it before

   fread(&dos_header, sizeof (dos_header), 1, fp); // get the DOS header
   fseek(fp, dos_header.e_lfanew, SEEK_SET);
   fread(&nt_signature, sizeof (nt_signature), 1, fp); // get the NT signature
   fread(&pe_header, sizeof (pe_header), 1, fp); // get the PE header
   fread(&optional_header, sizeof (optional_header), 1, fp); // get the optional header

   edata_offset = optional_header.DataDirectory[0].VirtualAddress; // no edata by default
   edata_delta = 0;

   // cycle through all sections of the PE header to look for edata
   for (i = 0; i < pe_header.NumberOfSections; i++)
      if (strcmp((char *) section_header.Name, ".edata") == 0)
      {
         edata_offset = section_header.PointerToRawData; // if found, save its offset
         edata_delta = section_header.VirtualAddress - section_header.PointerToRawData;
      }

   fseek(fp, edata_offset, SEEK_SET);
   fread(&export_directory, sizeof (export_directory), 1, fp); // get the export directory

   num_ordinals = export_directory.NumberOfNames; // save number of ordinals

   ordinal_offset = export_directory.AddressOfNameOrdinals - edata_delta; // save ordinals offset
   fseek(fp, ordinal_offset, SEEK_SET);
   p_Ordinals = (WORD *)malloc(num_ordinals * sizeof (WORD)); // allocate space for ordinals
   fread(p_Ordinals, num_ordinals * sizeof (WORD), 1, fp); // get the list of ordinals

   function_offset = export_directory.AddressOfFunctions - edata_delta; // save functions offset
   fseek(fp, function_offset, SEEK_SET);
   p_Functions = (DWORD *)malloc(num_ordinals * sizeof (DWORD)); // allocate space for functions
   fread(p_Functions, num_ordinals * sizeof (DWORD), 1, fp); // get the list of functions

   name_offset = export_directory.AddressOfNames - edata_delta; // save names offset
   fseek(fp, name_offset, SEEK_SET);
   p_Names = (DWORD *)malloc(num_ordinals * sizeof (DWORD)); // allocate space for names
   fread(p_Names, num_ordinals * sizeof (DWORD), 1, fp); // get the list of names

   p_FunctionNames = (char **)malloc(sizeof(char *) * num_ordinals);

   // reset function names array first
   for (i = 0; i < num_ordinals; i++)
      p_FunctionNames[i] = NULL;

   // cycle through all function names and fill in the exports array
   for (i = 0; i < num_ordinals; i++)
   {
      if (fseek(fp, p_Names[i] - edata_delta, SEEK_SET) != -1)
      {
         j = 0; // start at beginning of string

         // while end of file is not reached
         while ((ch = fgetc (fp)) != EOF)
         {
            function_name[j] = ch; // store what is read in the name variable
            if (ch == 0)
               break; // return the name with the trailing \0
            j++;
         }

         // allocate space
         p_FunctionNames[i] = (char *)malloc(strlen(function_name) + 1);

         if (function_name[0] == '?') // is this a MSVC mangled name?
         {
            j = 1; // skip the leading '?'

            // while the first @@ is not reached
            while (!((function_name[j] == '@') && (function_name[j + 1] == '@')))
            {
               p_FunctionNames[i][j - 1] = function_name[j]; // store what is read in the name variable
               if (function_name[j + 1] == 0)
                  break; // return the name
               j++;
            }

            p_FunctionNames[i][j] = 0; // terminate string at the "@@"
         }

         else // else no change needed
            strcpy (p_FunctionNames[i], function_name);
      }
   }

   fclose (fp); // close MOD DLL file

   // cycle through all function names to find the GiveFnptrsToDll function
   for (i = 0; i < num_ordinals; i++)
   {
      if (strcmp("GiveFnptrsToDll", p_FunctionNames[i]) == 0)
      {
         game_GiveFnptrsToDll = (void *)GetProcAddress(h_Library, "GiveFnptrsToDll");
         base_offset = (unsigned long)(game_GiveFnptrsToDll) - p_Functions[p_Ordinals[i]];
         break; // base offset has been saved
      }
   }
}

uint32 FunctionFromName(const char *pName)
{
   // this function returns the address of a certain function in the exports array. We don't call
   // the engine function for this, because since our bot DLL doesn't exports ALL the functions
   // the game DLL is exporting (functions beginning by "@@" are missing), we had to build our
   // own exports array by reading the game DLL file to complete the one of our bot DLL. That's
   // the purpose of the LoadSymbols() function, which is called as soon as LoadLibrary() is
   // called in GameDLLInit(). Note this fix is only enabled on Win32 systems, since UNIXes
   // don't need it (only hosting dedicated servers and thus never single player games).

   // cycle through our exports array and find the entry containing the function name we want
   for (int index = 0; index < num_ordinals; index++)
      if (strcmp(pName, p_FunctionNames[index]) == 0)
         return (p_Functions[p_Ordinals[index]] + base_offset); // return the address of that function

   // either no custom exports or couldn't find entry, fallback on asking the engine for it
   return g_engfuncs.pfnFunctionFromName(pName);
}


const char *NameForFunction(uint32 function)
{
   // this function returns the name of the function at a certain address in the exports array.
   // We don't call the engine function for this, because since our bot DLL doesn't exports ALL
   // the functions the game DLL is exporting (functions beginning by "@@" are missing), we had
   // to build our own exports array by reading the game DLL file to complete the one of our bot
   // DLL. That's the purpose of the LoadSymbols() function, which is called as soon as
   // LoadLibrary() is called in GameDLLInit(). Note this fix is only enabled on Win32 systems,
   // since UNIXes don't need it (only hosting dedicated servers and thus never single player
   // games).

   unsigned long address_to_look_for = function - base_offset;

   // cycle through our exports array and stop at the function offset index we want
   for (int index = 0; index < num_ordinals; index++)
      if (address_to_look_for == p_Functions[p_Ordinals[index]])
         return (p_FunctionNames[index]); // return the name of that function

   // either no custom exports or couldn't find entry, fallback on asking the engine for it
   return g_engfuncs.pfnNameForFunction(function);
}

#endif
