//
// intl.cpp
//

#include "bot.h"

typedef struct
{
   char *orig;
   char *tran;
} langentry_t;

static langentry_t lang[256];
static int g_iNumLang = 0;

void trim(char *str)
{
   int pos = 0;
   char *dest = str;

   // skip leading blanks
   while (str[pos] <= ' ' && str[pos] > 0)
      pos++;

   while (str[pos]) {
      *(dest++) = str[pos];
      pos++;
   }

   *(dest--) = '\0'; // store the null

   // remove trailing blanks
   while (dest >= str && *dest <= ' ' && *dest > 0)
      *(dest--) = '\0';
}

void InitLang()
{
   if (IS_DEDICATED_SERVER())
      return; // only use English for Dedicated server

   char str[256], buf[1024];
   GetGameDirectory(str);
   strcat(str, "/addons/amxmodx/configs/Dm_KD/dmpb/lang.txt");

   FILE *fp = fopen(str, "r");
   if (fp == NULL)
      return; // cannot open file

   enum { LANG_ORIG, LANG_TRAN, LANG_IDLE };
   int state = LANG_IDLE;

   memset(lang, 0, sizeof(lang));
   g_iNumLang = -1;

   buf[0] = '\0';

   while (fgets(str, 256, fp) != NULL)
   {
      if (strncmp(str, "[ORIGINAL]", 10) == 0)
      {
         state = LANG_ORIG;

         if (g_iNumLang >= 0)
         {
            trim(buf);
            lang[g_iNumLang].tran = strdup(buf);
            buf[0] = '\0';
         }

         if (g_iNumLang >= 255)
            break; // max entry reached

         g_iNumLang++;
      }
      else if (strncmp(str, "[TRANSLATED]", 12) == 0)
      {
         trim(buf);
         lang[g_iNumLang].orig = strdup(buf);
         buf[0] = '\0';
         state = LANG_TRAN;
      }
      else
      {
         switch (state)
         {
         case LANG_ORIG:
            assert(g_iNumLang >= 0);
            if (g_iNumLang >= 0)
               strncat(buf, str, 1024 - strlen(buf));
            break;

         case LANG_TRAN:
            assert(g_iNumLang >= 0);
            if (g_iNumLang >= 0)
               strncat(buf, str, 1024 - strlen(buf));
            break;
         }
      }
   }

   g_iNumLang++;
   if (g_iNumLang > 0)
   {
      trim(buf);
      lang[g_iNumLang - 1].tran = strdup(buf);
   }

   fclose(fp);
}

void FreeLang()
{
   for (int i = 0; i < g_iNumLang; i++) {
      free(lang[i].orig);
      free(lang[i].tran);
   }
   g_iNumLang = 0;
}

const char *tr(const char *s)
{
   static char str[1024];

   const char *p = s + strlen(s) - 1;
   while (p > s && *p == '\n')
      p--;
   if (p != s)
      p++;

   strncpy(str, s, 1024);
   trim(str);

   for (int i = 0; i < g_iNumLang; i++)
   {
      if (strcmp(str, lang[i].orig) == 0)
      {
         strncpy(str, lang[i].tran, 1024);
         if (p != s)
            strncat(str, p, 1024 - strlen(str));
         return str;
      }
   }

   return s; // cannot find this string in trans database
}

