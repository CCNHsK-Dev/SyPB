/*
 * Copyright (c) 2001-2006 Will Day <willday@hpgx.net>
 * See the file "dllapi.h" in this folder for full information
 */

// Simplified version by Wei Mingzhi

#ifndef PLINFO_H
#define PLINFO_H

typedef enum
{
   PT_NEVER = 0,
   PT_STARTUP,
   PT_CHANGELEVEL,
   PT_ANYTIME,
   PT_ANYPAUSE,
} PLUG_LOADTIME;


typedef struct
{
   char *ifvers;
   char *name;
   char *version;
   char *date;
   char *author;
   char *url;
   char *logtag;
   PLUG_LOADTIME loadable;
   PLUG_LOADTIME unloadable;
} plugin_info_t;
extern plugin_info_t Plugin_info;

typedef plugin_info_t *plid_t;

#define PLID   &Plugin_info

#endif
