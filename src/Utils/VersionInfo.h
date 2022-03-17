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

	VersionInfo() {}

	VersionInfo(WORD major) :
		_major(major) {}

	VersionInfo(WORD major, WORD minor) :
		_major(major), _minor(minor) {}

	VersionInfo(WORD major, WORD minor, WORD patch) :
		_major(major), _minor(minor), _patch(patch) {}

	VersionInfo(WORD major, WORD minor, WORD patch, WORD build) :
		_major(major), _minor(minor), _patch(patch), _build(build) {}

	VersionInfo(HMODULE fromModule) {
		LoadFromModule(fromModule);
	}

	VersionInfo(const char* fromPath) {
		HMODULE hModule = GetModuleHandle(str2wstr(std::string(fromPath)).c_str());
		LoadFromModule(hModule);
	}

	VersionInfo(const TCHAR* fromPath) {
		HMODULE hModule = GetModuleHandle(fromPath);
		LoadFromModule(hModule);
	}

	VersionInfo(std::string& fromPath) {
		HMODULE hModule = GetModuleHandle(str2wstr(fromPath.c_str()).c_str());
		LoadFromModule(hModule);
	}

	VersionInfo(generic_string fromPath) {
		HMODULE hModule = GetModuleHandle(fromPath.c_str());
		LoadFromModule(hModule);
	}

	bool operator ==(VersionInfo& other) {
		return
			_major == other._major &&
			_minor == other._minor &&
			_patch == other._patch &&
			_build == other._build;
	}

	bool operator >(VersionInfo& other) {
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

	bool operator <(VersionInfo& other) {
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

	// Extracts version information from a module resource
	static VersionInfo getVersionFromResource(HMODULE hModule);

	// Returns version information of the local module
	static VersionInfo getLocalVersion();

	// Extracts version information from a module binary path
	static VersionInfo getVersionFromBinary(generic_string modulePath);

private:

	WORD _major = 0;
	WORD _minor = 0;
	WORD _patch = 0;
	WORD _build = 0;

	void LoadFromModule(HMODULE moduleName);

	void setVersionString(std::string& value);

	// In case you don't know which module to load
	static HMODULE getThisModuleHandle();
};