/** @file VersionInfo.h
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

#pragma once

#include <Windows.h>

// If version resource renamed, change VS_VERSION_INFO to point to the new resource ID
#ifdef VS_VERSION_INFO
#define VERSION_POINTER VS_VERSION_INFO
#else
#define VERSION_POINTER 1
#endif

struct VersionInfo {

	// Constructors
	VersionInfo() {}

	// This constructor may require explicit type cast to WORD
	VersionInfo(WORD major) :
		_major(major) {}
	VersionInfo(WORD major, WORD minor) :
		_major(major), _minor(minor) {}
	VersionInfo(WORD major, WORD minor, WORD patch) :
		_major(major), _minor(minor), _patch(patch) {}
	VersionInfo(WORD major, WORD minor, WORD patch, WORD build) :
		_major(major), _minor(minor), _patch(patch), _build(build) {}
	VersionInfo(HMODULE fromModule) {
		LoadThisFromModule(fromModule);
	}
	// This constructor may require explicit type cast to char*
	VersionInfo(const char* versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(str2wstr(std::string(versionStringOrModulePath)).c_str());
	}
	// This constructor may require explicit type cast to TCHAR*
	VersionInfo(const TCHAR* versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(versionStringOrModulePath);
	}
	VersionInfo(std::string& versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(str2wstr(versionStringOrModulePath).c_str());
	}
	VersionInfo(std::wstring& versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(versionStringOrModulePath.c_str());
	}
	VersionInfo(std::string versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(str2wstr(versionStringOrModulePath).c_str());
	}
	VersionInfo(std::wstring versionStringOrModulePath) {
		LoadThisFromVersionStringOrPath(versionStringOrModulePath.c_str());
	}

	// This operator may require explicit type cast to WORD
	void operator =(WORD newVersion) {
		*this = VersionInfo(newVersion & 0xFFFF);
	}

	bool operator ==(VersionInfo& other) {
		return
			_major == other._major &&
			_minor == other._minor &&
			_patch == other._patch &&
			_build == other._build;
	}
	bool operator ==(const char* other) {
		return *this == VersionInfo(other);
	}
	bool operator ==(const TCHAR* other) {
		return *this == VersionInfo(other);
	}
	bool operator ==(const std::string& other) {
		return *this == VersionInfo(other);
	}
	bool operator ==(const std::wstring& other) {
		return *this == VersionInfo(other);
	}

	bool operator >(VersionInfo other) {
		if (_major > other._major)
			return true;
		if (_major < other._major)
			return false;

		// Major equals...
		if (_minor > other._minor)
			return true;
		if (_minor < other._minor)
			return false;

		// Minor also equals...
		if (_patch > other._patch)
			return true;
		if (_patch < other._patch)
			return false;

		// Patch also equals...
		if (_build > other._build)
			return true;
		else
			return false;
	}
	bool operator >(const char* other) {
		return *this > VersionInfo(other);
	}
	bool operator >(const TCHAR* other) {
		return *this > VersionInfo(other);
	}
	bool operator >(const std::string& other) {
		return *this > VersionInfo(other);
	}
	bool operator >(const std::wstring& other) {
		return *this > VersionInfo(other);
	}

	bool operator <(VersionInfo other) {
		if (_major < other._major)
			return true;
		if (_major > other._major)
			return false;

		// Major equals...
		if (_minor < other._minor)
			return true;
		if (_minor > other._minor)
			return false;

		// Minor also equals...
		if (_patch < other._patch)
			return true;
		if (_patch > other._patch)
			return false;

		// Patch also equals...
		if (_build < other._build)
			return true;
		else
			return false;
	}
	bool operator <(const char* other) {
		return *this < VersionInfo(other);
	}
	bool operator <(const TCHAR* other) {
		return *this < VersionInfo(other);
	}
	bool operator <(const std::string& other) {
		return *this < VersionInfo(other);
	}
	bool operator <(const std::wstring& other) {
		return *this < VersionInfo(other);
	}

	bool operator <=(VersionInfo other) {
		return !(*this > other);
	}
	bool operator <=(const char* other) {
		return !(*this > other);
	}
	bool operator <=(const TCHAR* other) {
		return !(*this > other);
	}
	bool operator <=(const std::string& other) {
		return !(*this > other);
	}
	bool operator <=(const std::wstring& other) {
		return !(*this > other);
	}

	bool operator >=(VersionInfo other) {
		return !(*this < other);
	}
	bool operator >=(const char* other) {
		return !(*this < other);
	}
	bool operator >=(const TCHAR* other) {
		return !(*this < other);
	}
	bool operator >=(const std::string& other) {
		return !(*this < other);
	}
	bool operator >=(const std::wstring& other) {
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
	static VersionInfo getVersionFromModuleHandle(HMODULE hModule);
	// Returns version information of the local module
	static VersionInfo getLocalVersion();
	// Extracts version information from a module binary path
	static VersionInfo getVersionFromBinary(generic_string modulePath);

private:

	WORD _major = 0;
	WORD _minor = 0;
	WORD _patch = 0;
	WORD _build = 0;

	// Load version from version string (eg: 1.0.0.0) - may fail to parse string.
	bool LoadThisFromVersionString(const TCHAR* versionString);

	// Load version from module handle.
	void LoadThisFromModule(HMODULE moduleName);

	// Load version from version string or binary path.
	void LoadThisFromVersionStringOrPath(const TCHAR* versionStringOrModulePath);

	// Returns this module handle. Required to be static.
	static HMODULE getThisModuleHandle();
};