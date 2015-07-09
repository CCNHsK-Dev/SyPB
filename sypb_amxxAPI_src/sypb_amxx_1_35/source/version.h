
#ifndef RESOURCE_INCLUDED
#define RESOURCE_INCLUDED

// SyPB AMXX API Version
#define PRODUCT_VERSION "1.35"
float dllVersion = 1.35;

#define PRODUCT_VERSION_DWORD 1,35,20150601,31    // yyyy/mm/dd   

// general product information
#define PRODUCT_NAME "SyPB AMXX DLL"
#define PRODUCT_AUTHOR "HsK Dev-Blog @ CCN"
#define PRODUCT_LOGTAG "SyPB"
#define PRODUCT_DESCRIPTION PRODUCT_NAME " Version: v" PRODUCT_VERSION
#define PRODUCT_COPYRIGHT "by " PRODUCT_AUTHOR
#define PRODUCT_LEGAL "SyPB, " PRODUCT_AUTHOR

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

