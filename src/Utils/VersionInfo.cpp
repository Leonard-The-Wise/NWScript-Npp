/** @file VersionInfo.cpp
 * Get version information from resource file.
 * 
 * Original idea extracted from simple example here:
 * https://iq.direct/blog/336-code-sample-retrieve-c-application-version-from-vs-version-resource.html
 *
 * To use this file, you MUST add "version.lib" to your project Linker's "Aditional Dependencies"
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
#include "Common.h"
#include "jpcre2.hpp"

#include "VersionInfo.h"

static const jpcre2::select<char>::Regex number(R"(\d+)", 0, jpcre2::JIT_COMPILE);
static const jpcre2::select<char>::Regex validversion(R"(^(\d+)\.?(?:(\d+)\.?)?(?:(\d+)\.?)?(?:(\d+)\.?)?$)", 0, jpcre2::JIT_COMPILE);
static jpcre2::select<char>::RegexMatch validator(&validversion);
static jpcre2::select<char>::RegexMatch numbers(&number);

void VersionInfo::setVersionString(std::string& value)
{
	jpcre2::select<char>::VecNum versions;
	int count = validator.setNumberedSubstringVector(&versions).setSubject(value).match();
	if (count == 0)
		return;

	numbers.setNumberedSubstringVector(&versions).setSubject(value).match();
	if (count > 0)
	{
		_major = stoi(versions[0][1]) & 0xFFFF;
		if (!versions[0][2].empty())
			_minor = stoi(versions[0][2]) & 0xFFFF;
		if (!versions[0][3].empty())
			_patch = stoi(versions[0][3]) & 0xFFFF;
		if (!versions[0][4].empty())
			_build = stoi(versions[0][4]) & 0xFFFF;
	}
}

// Extracts version information from a Executable or DLL binary
VersionInfo VersionInfo::getVersionFromResource(HMODULE hModule)
{
	if (!hModule)
		return VersionInfo();

	HRSRC hResInfo;
	DWORD dwSize;
	HGLOBAL hResData;
	LPVOID pRes, pResCopy;
	UINT uLen = 0;
	VS_FIXEDFILEINFO* lpFfi = NULL;
	HINSTANCE hInst = reinterpret_cast<HINSTANCE>(hModule);
	VersionInfo VersionInfo;

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
							VersionInfo = { HIWORD(lpFfi->dwFileVersionMS), LOWORD(lpFfi->dwFileVersionMS),
								HIWORD(lpFfi->dwFileVersionLS), LOWORD(lpFfi->dwFileVersionLS) };
						}
					}
					LocalFree(pResCopy);
				}
			}
		}
	}

	return VersionInfo;
}

// Extracts version information from this executable/binary
VersionInfo VersionInfo::getLocalVersion()
{
	HMODULE hModule = getThisModuleHandle();
	return getVersionFromResource(hModule);
}

// Extracts version information from a given binary/executable
VersionInfo VersionInfo::getVersionFromBinary(generic_string modulePath) {

	HMODULE hModule = GetModuleHandle(modulePath.c_str());
	return getVersionFromResource(hModule);
}

// In case we don't know which module to search
HMODULE VersionInfo::getThisModuleHandle()
{
	//Returns module handle where this function is running in: EXE or DLL
	HMODULE hModule = NULL;
	::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)getThisModuleHandle, &hModule);

	return hModule;
}

// Load version from module
void VersionInfo::LoadFromModule(HMODULE moduleName) {
	VersionInfo myVersion = getVersionFromResource(moduleName);
	_major = myVersion._major;
	_minor = myVersion._minor;
	_patch = myVersion._patch;
	_build = myVersion._build;
}
