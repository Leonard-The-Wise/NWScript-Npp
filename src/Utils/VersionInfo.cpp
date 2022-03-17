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

static const jpcre2::select<wchar_t>::Regex number(TEXT(R"(\d+)"), 0, jpcre2::JIT_COMPILE | jpcre2::FIND_ALL);
static const jpcre2::select<wchar_t>::Regex validversion(TEXT(R"(^(\d+)\.?(?:(\d+)\.?)?(?:(\d+)\.?)?(?:(\d+)\.?)?$)"), 0, jpcre2::JIT_COMPILE);
static jpcre2::select<wchar_t>::RegexMatch validator(&validversion);
static jpcre2::select<wchar_t>::RegexMatch numbers(&number);

// Load version from version string
bool VersionInfo::LoadThisFromVersionString(const TCHAR* versionString)
{
	jpcre2::select<wchar_t>::VecNum version;
	std::wstring subject(versionString);
	int count = validator.setNumberedSubstringVector(&version).setSubject(subject).match();
	if (count == 0)
		return false;

	count = numbers.setNumberedSubstringVector(&version).setSubject(subject).setModifier("g").match();
	if (count > 0)
	{
		_major = stoi(version[0][0]) & 0xFFFF;
		if (count > 1)
			_minor = stoi(version[1][0]) & 0xFFFF;
		if (count > 2)
			_patch = stoi(version[2][0]) & 0xFFFF;
		if (count > 3)
			_build = stoi(version[3][0]) & 0xFFFF;
	}

	return true;
}

// Load version from module
void VersionInfo::LoadThisFromModule(HMODULE moduleName) {
	VersionInfo myVersion = getVersionFromModuleHandle(moduleName);
	_major = myVersion._major;
	_minor = myVersion._minor;
	_patch = myVersion._patch;
	_build = myVersion._build;
}

// Load version from version string or module path
void VersionInfo::LoadThisFromVersionStringOrPath(const TCHAR* versionStringOrModulePath)
{
	// First try to set strings directly. If fail, try loading modules
	if (!LoadThisFromVersionString(versionStringOrModulePath))
	{
		HMODULE hModule = GetModuleHandle(versionStringOrModulePath);
		if (hModule)
			LoadThisFromModule(hModule);
	}
}



// Static (class) functions

// In case we don't know which module to search
// Static private (required for GetModuleHandleEx call)
HMODULE VersionInfo::getThisModuleHandle()
{
	//Returns module handle where this function is running in: EXE or DLL
	HMODULE hModule = NULL;
	::GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
		GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
		(LPCTSTR)getThisModuleHandle, &hModule);

	return hModule;
}

// Extracts version information from a Executable or DLL binary
VersionInfo VersionInfo::getVersionFromModuleHandle(HMODULE hModule)
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
	return getVersionFromModuleHandle(hModule);
}

// Extracts version information from a given binary/executable
VersionInfo VersionInfo::getVersionFromBinary(generic_string modulePath) {

	HMODULE hModule = GetModuleHandle(modulePath.c_str());
	return getVersionFromModuleHandle(hModule);
}




