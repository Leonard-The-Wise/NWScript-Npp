/** @file VersionResource.h
 * Get version information from resource file.
 * Extracted and modified from example here: 
 * https://iq.direct/blog/336-code-sample-retrieve-c-application-version-from-vs-version-resource.html
 * 
 * To use this file, you MUST add "version.lib" to your project Linker's "Aditional Dependencies"
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <Windows.h>

// If version resource renamed, change VS_VERSION_INFO to point to the new resource ID
#ifdef VS_VERSION_INFO
#define VERSION_POINTER VS_VERSION_INFO
#else
#define VERSION_POINTER 1
#endif


struct DllVersion {
	DWORD dwFileVersionMS = 0;
	DWORD dwFileVersionLS = 0;

	WORD wLeftMost = 0;
	WORD wSecondLeft = 0;
	WORD wSecondRight = 0;
	WORD wRightMost = 0;
	DllVersion() {}
	DllVersion(DWORD _ms, DWORD _ls, WORD _lm, WORD _sl, WORD _sr, WORD _rm) :
		dwFileVersionMS(_ms), dwFileVersionLS(_ls), wLeftMost(_lm), wSecondLeft(_sl),
		wSecondRight(_sr), wRightMost(_rm)
	{}
};

// In case you don't know which module to load
HMODULE getThisModuleHandle();

DllVersion GetVersionFromResource(HMODULE hModule = nullptr);

DllVersion GetVersionFromLocalResource();