/** @file VersionResource.h
 * Get version information from resource file.
 * Extracted and modified from example here: 
 * https://iq.direct/blog/336-code-sample-retrieve-c-application-version-from-vs-version-resource.html
 * 
 * To use this file, you MUST add "version.lib" to your project Linker's "Aditional Dependencies"
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
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
HMODULE getThisModuleHandle()
{
	//Returns module handle where this function is running in: EXE or DLL
	HMODULE hModule = NULL;
	::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)getThisModuleHandle, &hModule);

	return hModule;
}

DllVersion GetVersionFromResource(HMODULE hModule)
{
	HRSRC hResInfo;
	DWORD dwSize;
	HGLOBAL hResData;
	LPVOID pRes, pResCopy;
	UINT uLen = 0;
	VS_FIXEDFILEINFO* lpFfi = NULL;
	HINSTANCE hInst = hModule; 
	DllVersion VersionInfo;

	hResInfo = FindResource(hInst, reinterpret_cast<LPCWSTR>(MAKEINTRESOURCE(VERSION_POINTER)), RT_VERSION);
	if (hResInfo)
	{
		dwSize = SizeofResource(hInst, hResInfo);
		hResData = LoadResource(hInst, hResInfo);
		if (hResData)
		{
			pRes = LockResource(hResData);
			if (pRes)
			{
				pResCopy = LocalAlloc(LMEM_FIXED, dwSize);
				if (pResCopy)
				{
					CopyMemory(pResCopy, pRes, dwSize);

					if (VerQueryValue(pResCopy, L"\\", reinterpret_cast<LPVOID*>(&lpFfi), &uLen))
					{
						if (lpFfi != NULL)
						{
							VersionInfo = DllVersion(lpFfi->dwFileVersionMS, lpFfi->dwFileVersionLS,
								HIWORD(lpFfi->dwFileVersionMS), LOWORD(lpFfi->dwFileVersionMS),
								HIWORD(lpFfi->dwFileVersionLS), LOWORD(lpFfi->dwFileVersionLS));
						}
					}
					LocalFree(pResCopy);
				}
			}
		}
	}

	return VersionInfo;
}

DllVersion GetVersionFromLocalResource()
{
	HMODULE hModule = getThisModuleHandle();
	return GetVersionFromResource(hModule);
}
