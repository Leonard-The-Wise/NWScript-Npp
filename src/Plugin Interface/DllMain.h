/** @file DllMain.h
 * Definitions for DLL API Exports
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#pragma once

#if (defined(_USRDLL) || defined(DLL_EXPORTS)) && defined(_LIB)
#error You cannot compile this code defining it both as a static and dynamic library simultaneously!
#endif

#if defined(_USRDLL) || defined(DLL_EXPORTS)
#define DLLAPI __declspec(dllexport)
#elif !defined(_LIB)
#define DLLAPI __declspec(dllimport)
#endif

