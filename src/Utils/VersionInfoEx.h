/** @file VersionInfoEx.h
 * Get version information from resource file.
 * 
 * Original idea extracted from simple example here:
 * https://iq.direct/blog/336-code-sample-retrieve-c-application-version-from-vs-version-resource.html
 * 
 * To use this file, you MUST add "version.lib" to your linker dependencies
 * (that's what the pragma comment down there does)
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#pragma comment (lib, "version")

#include <Windows.h>
#include "Common.h"

// If version resource renamed, change VS_VERSION_INFO to point to the new resource ID
#ifdef VS_VERSION_INFO
#define VERSION_POINTER VS_VERSION_INFO
#else
#define VERSION_POINTER 1
#endif

struct VersionInfoEx {

	// Constructors
	VersionInfoEx() {}

	VersionInfoEx(WORD major) :
		_major(major) {}
	VersionInfoEx(WORD major, WORD minor) :
		_major(major), _minor(minor) {}
	VersionInfoEx(WORD major, WORD minor, WORD patch) :
		_major(major), _minor(minor), _patch(patch) {}
	VersionInfoEx(WORD major, WORD minor, WORD patch, WORD build) :
		_major(major), _minor(minor), _patch(patch), _build(build) {}
	VersionInfoEx(const HMODULE fromModule) {
		LoadThisFromModule(fromModule);
	}
	VersionInfoEx(const char* versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(str2wstr(std::string(versionStringOrModulePath)).c_str());
	}
	VersionInfoEx(const TCHAR* versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(versionStringOrModulePath);
	}
	VersionInfoEx(const std::string& versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(str2wstr(versionStringOrModulePath).c_str());
	}
	VersionInfoEx(const std::wstring& versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(versionStringOrModulePath.c_str());
	}

	void operator=(WORD newVersion) {
		*this = VersionInfoEx(newVersion & 0xFFFF);
	}

	bool operator==(const VersionInfoEx& other) const {
		return (((UINT64)_major << 48) + ((UINT64)_minor << 32) + ((UINT64)_patch << 16) + ((UINT64)_build)) ==
			(((UINT64)other._major << 48) + ((UINT64)other._minor << 32) + ((UINT64)other._patch << 16) + ((UINT64)other._build));
	}
	bool operator==(const char* other) const {
		return *this == VersionInfoEx(other);
	}
	bool operator==(const TCHAR* other) const {
		return *this == VersionInfoEx(other);
	}
	bool operator==(const std::string& other) const {
		return *this == VersionInfoEx(other);
	}
	bool operator==(const std::wstring& other) const {
		return *this == VersionInfoEx(other);
	}

	bool operator>(const VersionInfoEx& other) const {
		return (((UINT64)_major << 48) + ((UINT64)_minor << 32) + ((UINT64)_patch << 16) + ((UINT64)_build)) >
		(((UINT64)other._major << 48) + ((UINT64)other._minor << 32) + ((UINT64)other._patch << 16) + ((UINT64)other._build));
	}
	bool operator>(const char* other) const {
		return *this > VersionInfoEx(other);
	}
	bool operator>(const TCHAR* other) const {
		return *this > VersionInfoEx(other);
	}
	bool operator>(const std::string& other) const {
		return *this > VersionInfoEx(other);
	}
	bool operator>(const std::wstring& other) const {
		return *this > VersionInfoEx(other);
	}

	bool operator<(const VersionInfoEx& other) const {
		return (((UINT64)_major << 48) + ((UINT64)_minor << 32) + ((UINT64)_patch << 16) + ((UINT64)_build)) <
		(((UINT64)other._major << 48) + ((UINT64)other._minor << 32) + ((UINT64)other._patch << 16) + ((UINT64)other._build));
	}
	bool operator<(const char* other) const {
		return *this < VersionInfoEx(other);
	}
	bool operator<(const TCHAR* other) const {
		return *this < VersionInfoEx(other);
	}
	bool operator<(const std::string& other) const {
		return *this < VersionInfoEx(other);
	}
	bool operator<(const std::wstring& other) const {
		return *this < VersionInfoEx(other);
	}

	bool operator<=(const VersionInfoEx& other) const {
		return !(*this > other);
	}
	bool operator<=(const char* other) const {
		return !(*this > other);
	}
	bool operator<=(const TCHAR* other) const {
		return !(*this > other);
	}
	bool operator<=(const std::string& other) const {
		return !(*this > other);
	}
	bool operator<=(const std::wstring& other) const {
		return !(*this > other);
	}

	bool operator>=(const VersionInfoEx& other) const {
		return !(*this < other);
	}
	bool operator>=(const char* other) const {
		return !(*this < other);
	}
	bool operator>=(const TCHAR* other) const {
		return !(*this < other);
	}
	bool operator>=(const std::string& other) const {
		return !(*this < other);
	}
	bool operator>=(const std::wstring& other) const {
		return !(*this < other);
	}

	WORD major() const {
		return _major;
	}

	WORD minor() const {
		return _minor;
	}

	WORD patch() const {
		return _patch;
	}

	WORD build() const {
		return _build;
	}

	bool empty() const {
		return _major == 0 && _minor == 0 && _patch == 0 && _build == 0;
	}

	// Returns module complete string (ex: 1.0.0.0)
	std::string string() const {
		return std::string(std::to_string(this->_major) + "." + std::to_string(this->_minor) + "."
			+ std::to_string(this->_patch) + "." + std::to_string(this->_build));
	}
	// Returns module shortened string (ex: 1.0.0)
	std::string shortString() const {
		return std::string(std::to_string(this->_major) + "." + std::to_string(this->_minor) + "."
			+ std::to_string(this->_patch));
	}
	// Returns module double shortened string (ex: 1.0)
	std::string doubleShortString() const {
		return std::string(std::to_string(this->_major) + "." + std::to_string(this->_minor));
	}

	// Returns module complete string (ex: 1.0.0.0)
	std::wstring wstring() const {
		return str2wstr(string());
	}
	// Returns module shortened string (ex: 1.0.0)
	std::wstring wshortString() const {
		return str2wstr(shortString());
	}
	// Returns module double shortened string (ex: 1.0)
	std::wstring wdoubleShortString() const {
		return str2wstr(doubleShortString());
	}

	// Static (class) functions

	// Extracts version information from a module resource
	static VersionInfoEx getVersionFromModuleHandle(HMODULE hModule);
	// Returns version information of the local module
	static VersionInfoEx getLocalVersion();
	// Extracts version information from a module binary path
	static VersionInfoEx getVersionFromBinary(generic_string modulePath);

private:

	WORD _major = 0;
	WORD _minor = 0;
	WORD _patch = 0;
	WORD _build = 0;

	// Load version from version string (eg: 1.0.0.0) - may fail to parse string.  Internal use...
	bool LoadThisFromVersionString(const TCHAR* versionString);

	// Load version from module handle.  Internal use...
	void LoadThisFromModule(HMODULE moduleName);

	// Load version from version string or binary path.  Internal use...
	void LoadThisFromVersionStringOrPath(const TCHAR* versionStringOrModulePath);

	// Returns this module handle. Required to be static.
	static HMODULE getThisModuleHandle();
};
