/** @file VersionResource.cpp
 * Get version information from resource file.
 * Extracted and modified from example here:
 * https://iq.direct/blog/336-code-sample-retrieve-c-application-version-from-vs-version-resource.html
 *
 * To use this file, you MUST add "version.lib" to your project Linker's "Aditional Dependencies"
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
#include "VersionFromResource.h"


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
	HINSTANCE hInst = reinterpret_cast<HINSTANCE>(hModule);
	DllVersion VersionInfo;

	if (!hInst)
		hInst = reinterpret_cast<HINSTANCE>(getThisModuleHandle());

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
