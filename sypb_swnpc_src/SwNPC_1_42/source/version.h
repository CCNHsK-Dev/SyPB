

#ifndef RESOURCE_INCLUDED
#define RESOURCE_INCLUDED

#define PRODUCT_VERSION "Beta 1.42 - Upgrade 1"
#define PRODUCT_VERSION_F 1.42
#define PRODUCT_VERSION_DWORD 1,42,20160316,101 // yyyy/mm/dd   

#define PRODUCT_DEV_VERSION_FORTEST "(DEV)"
//#define PRODUCT_DEV_VERSION_FORTEST "(Preview-1)"
//#define PRODUCT_DEV_VERSION_FORTEST ""

// general product information
#define PRODUCT_NAME "SwNPC"
#define PRODUCT_AUTHOR "HsK Dev-Blog @ CCN"
#define PRODUCT_LOGTAG "SwNPC"
#define PRODUCT_DESCRIPTION PRODUCT_NAME " v" PRODUCT_VERSION PRODUCT_DEV_VERSION_FORTEST
#define PRODUCT_COPYRIGHT PRODUCT_AUTHOR
#define PRODUCT_LEGAL "SwNPC, " PRODUCT_AUTHOR
#define PRODUCT_URL "http://ccnhsk-dev.blogspot.com/"

// product optimization type (we're not using crt builds anymore)
#ifndef PRODUCT_OPT_TYPE
#if defined (_DEBUG)
#   if defined (_AFXDLL)
#      define PRODUCT_OPT_TYPE "Debug Build (CRT)"
#   else
#      define PRODUCT_OPT_TYPE "Debug Build"
#   endif
#elif defined (NDEBUG)
#   if defined (_AFXDLL)
#      define PRODUCT_OPT_TYPE "Optimized Build (CRT)"
#   else
#      define PRODUCT_OPT_TYPE "Optimized Build"
#   endif
#else
#   define PRODUCT_OPT_TYPE "Default Release"
#endif
#endif

#endif // RESOURCE_INCLUDED

