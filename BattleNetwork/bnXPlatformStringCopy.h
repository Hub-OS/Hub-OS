#pragma once

#ifdef __ANDROID__
#define USE_STRCPY_S 1
#endif

#ifdef __APPLE__
#define USE_STRCPY_S 1
#endif

#ifndef USE_STRCPY_S
#define USE_STRCPY_S 0
#endif

#ifdef USE_STRCPY_S
#define XPLATFORM_STRCPY(dest, len, src) strcpy_s(dest, len, src)
#else
#define XPLATFORM_STRCPY(dest, len, src) strcpy(dest, src)
#endif