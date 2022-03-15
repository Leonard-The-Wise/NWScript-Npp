/** @file PluginMain.cpp
 * Controls the Plugin settings
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <algorithm>
//#include <cctype>
//#include <string>
//#include <tchar.h>
//#include <sstream>
//#include <iomanip>


#include "Settings.h"

#define PATHSPLITREGEX TEXT(R"(\"?([^;"]+)\"?;?)")
#define FILTERSREGEX TEXT(R"(([^,]+))")

jpcre2::select<TCHAR>::Regex dirRegex(PATHSPLITREGEX, 0, jpcre2::FIND_ALL | jpcre2::JIT_COMPILE);
jpcre2::select<TCHAR>::Regex filtersRegex(FILTERSREGEX, 0, jpcre2::FIND_ALL | jpcre2::JIT_COMPILE);

using namespace NWScriptPlugin;

void Settings::Load()
{
	iniFilePath->read(*iniFile);

	// Try to read if INI file was valid/existent prior to this load. If not, flush any unclear results, keep default values.
	_bValidINI = GetBoolean(TEXT("Plugin Startup"), TEXT("_bValidINI"));
	if (!_bValidINI)
	{
		iniFile->clear();
		return;
	}

	// Load all settings variables from INI here
	enableAutoIndentation = GetBoolean(TEXT("Plugin Functions"), TEXT("enableAutoIndentation"));
	autoIndentationWarningAccepted = GetBoolean(TEXT("Plugin Functions"), TEXT("autoIndentationWarningAccepted"));
	notepadRestartMode = static_cast<RestartMode>(GetNumber<int>(TEXT("Notepad Restart"), TEXT("notepadRestartMode")));
	notepadRestartFunction = static_cast<RestartFunctionHook>(GetNumber<int>(TEXT("Notepad Restart"), TEXT("notepadRestartFunction")));

	// Compiler settings
	compilerSettingsCreated = GetBoolean(TEXT("Compiler Settings"), TEXT("compilerSettingsCreated"));
	neverwinterInstallChoice = GetNumber<int>(TEXT("Compiler Settings"), TEXT("neverwinterInstallChoice"));
	neverwinterOneInstallDir = properDirName(GetString(TEXT("Compiler Settings"), TEXT("neverwinterOneInstallDir")));
	neverwinterTwoInstallDir = properDirName(GetString(TEXT("Compiler Settings"), TEXT("neverwinterTwoInstallDir")));
	ignoreInstallPaths = GetBoolean(TEXT("Compiler Settings"), TEXT("ignoreInstallPaths"));
	additionalIncludeDirs = GetString(TEXT("Compiler Settings"), TEXT("additionalIncludeDirs"));
	compilerFlags = GetNumber<int>(TEXT("Compiler Settings"), TEXT("compilerFlags"));
	optimizeScript = GetBoolean(TEXT("Compiler Settings"), TEXT("optimizeScript"));
	useNonBiowareExtenstions = GetBoolean(TEXT("Compiler Settings"), TEXT("useNonBiowareExtenstions"));
	generateSymbols = GetBoolean(TEXT("Compiler Settings"), TEXT("generateSymbols"));
	compileVersion = GetNumber<int>(TEXT("Compiler Settings"), TEXT("compileVersion"));
	useScriptPathToCompile = GetBoolean(TEXT("Compiler Settings"), TEXT("useScriptPathToCompile"));
	outputCompileDir = properDirName(GetString(TEXT("Compiler Settings"), TEXT("outputCompileDir")));

	// Batch Process
	startingBatchFolder = properDirName(GetString(TEXT("Batch Processing"), TEXT("startingBatchFolder")));
	fileFiltersCompile = GetString(TEXT("Batch Processing"), TEXT("fileFiltersCompile"));
	fileFiltersDisasm = GetString(TEXT("Batch Processing"), TEXT("fileFiltersDisasm"));
	compileMode = GetNumber<int>(TEXT("Batch Processing"), TEXT("compileMode"));
	recurseSubFolders = GetBoolean(TEXT("Batch Processing"), TEXT("recurseSubFolders"));
	continueCompileOnFail = GetBoolean(TEXT("Batch Processing"), TEXT("continueCompileOnFail"));
	useScriptPathToBatchCompile = GetBoolean(TEXT("Batch Processing"), TEXT("useScriptPathToBatchCompile"));
	batchOutputCompileDir = properDirName(GetString(TEXT("Batch Processing"), TEXT("batchOutputCompileDir")));

	// Sanity checks: avoid loading missing or corrupted data for compiled settings. Mark configurations invalid if inconsistency detected.
	if (!isValidDirectoryS(neverwinterOneInstallDir))
	{
		neverwinterOneInstallDir = TEXT("");
		compilerSettingsCreated = false;
	}
	if (!isValidDirectoryS(neverwinterTwoInstallDir))
	{
		neverwinterTwoInstallDir = TEXT("");
		compilerSettingsCreated = false;
	}

	generic_string validPaths;
	for (generic_string s : getIncludeDirsV())
	{
		if (isValidDirectoryS(s))
			validPaths.append(s).append(TEXT(";"));
		else
			compilerSettingsCreated = false;
	}

	additionalIncludeDirs = validPaths;

	if (compileVersion != 174 && compileVersion != 169)
	{
		compileVersion = 174;
		compilerSettingsCreated = false;
	}

	if (!isValidDirectoryS(outputCompileDir))
	{
		outputCompileDir = TEXT("");
		compilerSettingsCreated = false;
	}

	// We aren't checking batch compiling settings here, since the user will have to run the Dialog first to run a batch...

}

void Settings::Save()
{
	// Set this file as valid
	SetBoolean(TEXT("Plugin Startup"), TEXT("_bValidINI"), true);

	// Set all settings variables to INI here
	SetBoolean(TEXT("Plugin Functions"), TEXT("enableAutoIndentation"), enableAutoIndentation);
	SetBoolean(TEXT("Plugin Functions"), TEXT("autoIndentationWarningAccepted"), autoIndentationWarningAccepted);
	SetNumber<int>(TEXT("Notepad Restart"), TEXT("notepadRestartMode"), static_cast<int>(notepadRestartMode));
	SetNumber<int>(TEXT("Notepad Restart"), TEXT("notepadRestartFunction"), static_cast<int>(notepadRestartFunction));

	// Compiler settings
	SetBoolean(TEXT("Compiler Settings"), TEXT("compilerSettingsCreated"), compilerSettingsCreated);
	SetNumber<int>(TEXT("Compiler Settings"), TEXT("neverwinterInstallChoice"), neverwinterInstallChoice);
	SetString(TEXT("Compiler Settings"), TEXT("neverwinterOneInstallDir"), neverwinterOneInstallDir);
	SetString(TEXT("Compiler Settings"), TEXT("neverwinterTwoInstallDir"), neverwinterTwoInstallDir);
	SetBoolean(TEXT("Compiler Settings"), TEXT("ignoreInstallPaths"), ignoreInstallPaths);
	SetString(TEXT("Compiler Settings"), TEXT("additionalIncludeDirs"), additionalIncludeDirs);
	SetNumber<int>(TEXT("Compiler Settings"), TEXT("compilerFlags"), compilerFlags);
	SetBoolean(TEXT("Compiler Settings"), TEXT("optimizeScript"), optimizeScript);
	SetBoolean(TEXT("Compiler Settings"), TEXT("useNonBiowareExtenstions"), useNonBiowareExtenstions);
	SetBoolean(TEXT("Compiler Settings"), TEXT("generateSymbols"), generateSymbols);
	SetNumber<int>(TEXT("Compiler Settings"), TEXT("compileVersion"), compileVersion);
	SetBoolean(TEXT("Compiler Settings"), TEXT("useScriptPathToCompile"), useScriptPathToCompile);
	SetString(TEXT("Compiler Settings"), TEXT("outputCompileDir"), outputCompileDir);

	// Batch Process
	SetString(TEXT("Batch Processing"), TEXT("startingBatchFolder"), startingBatchFolder);
	SetString(TEXT("Batch Processing"), TEXT("fileFiltersCompile"), fileFiltersCompile);
	SetString(TEXT("Batch Processing"), TEXT("fileFiltersDisasm"), fileFiltersDisasm);
	SetNumber<int>(TEXT("Batch Processing"), TEXT("compileMode"), compileMode);
	SetBoolean(TEXT("Batch Processing"), TEXT("recurseSubFolders"), recurseSubFolders);
	SetBoolean(TEXT("Batch Processing"), TEXT("continueCompileOnFail"), continueCompileOnFail);
	SetBoolean(TEXT("Batch Processing"), TEXT("useScriptPathToBatchCompile"), useScriptPathToBatchCompile);
	SetString(TEXT("Batch Processing"), TEXT("batchOutputCompileDir"), batchOutputCompileDir);

	iniFilePath->write(*iniFile);
}

std::vector<generic_string> Settings::getIncludeDirsV() {
	return string2VectorRegex(additionalIncludeDirs, dirRegex);
}

std::vector<generic_string> Settings::getFileFiltersCompileV() {
	return string2VectorRegex(fileFiltersCompile, filtersRegex);
}

std::vector<generic_string> Settings::getFileFiltersDisasmV() {
	return string2VectorRegex(fileFiltersDisasm, filtersRegex);
}

void Settings::setIncludeDirs(const std::vector<generic_string>& newDirs) 
{
	additionalIncludeDirs = TEXT("");
	for (const generic_string& m : newDirs)
		additionalIncludeDirs.append(m);
}

void Settings::setFileFiltersCompile(const std::vector<generic_string>& newFilters)
{
	fileFiltersCompile = TEXT("");
	for (const generic_string& m : newFilters)
		fileFiltersCompile.append(m);
}

void Settings::setFileFiltersDisasm(const std::vector<generic_string>& newFilters)
{
	fileFiltersDisasm = TEXT("");
	for (const generic_string& m : newFilters)
		fileFiltersDisasm.append(m);
}

std::vector<generic_string> Settings::string2VectorRegex(const generic_string& target, const jpcre2::select<TCHAR>::Regex& separator)
{
	std::vector<generic_string> results;
	jpcre2::select<TCHAR>::VecNum matchr;
	jpcre2::select<TCHAR>::RegexMatch matcher(&separator);
	matcher.setNumberedSubstringVector(&matchr).setSubject(target).setModifier("g").match();
	
	for (std::vector<generic_string> m : matchr)
	{
		if (separator.getPattern() == PATHSPLITREGEX)
			results.push_back(properDirName(m[1]));
		else
			results.push_back(m[1]);
	}

	return results;
}


#pragma region 

// Try to get a number from INI. Returns 0 if not a number or don't exist
template <typename T>
T Settings::GetNumber(const generic_string& section, const generic_string& key)
{
	generic_stringstream rValue = {};
	rValue << (*iniFile)[section.c_str()][key.c_str()];
	if (rValue.str().empty())
		return 0;

	T xNumber = 0;
	rValue << std::fixed;
	rValue << std::setprecision(std::numeric_limits<T>::digits10);
	rValue >> xNumber;

	return xNumber;
}

bool Settings::GetBoolean(const generic_string& section, const generic_string& key)
{
	generic_string rValue = (*iniFile)[section.c_str()][key.c_str()];

	// False on empty or inexistent section
	if (rValue.empty())
		return false;

	// Try to get from TRUE or FALSE types of values
	std::transform(rValue.begin(), rValue.end(), rValue.begin(), ::towlower);
	if (_tcscmp(rValue.c_str(), TEXT("false")) == 0)
		return false;
	if (_tcscmp(rValue.c_str(), TEXT("true")) == 0)
		return true;

	// Return from a number
	return static_cast<bool>(GetNumber<int>(section, key));
}

inline generic_string Settings::GetString(const generic_string& section, const generic_string& key)
{
	return (*iniFile)[section.c_str()][key.c_str()];
}

template <typename T>
void Settings::SetNumber(const generic_string& section, const generic_string& key, T value)
{
	generic_stringstream sInput;
	sInput << std::fixed;
	sInput << std::setprecision(std::numeric_limits<T>::digits10);
	sInput << value;
	(*iniFile)[section.c_str()][key.c_str()] = sInput.str().c_str();
}

void Settings::SetBoolean(const generic_string& section, const generic_string& key, bool value)
{
	if (value)
		(*iniFile)[section.c_str()][key.c_str()] = TEXT("true");
	else
		(*iniFile)[section.c_str()][key.c_str()] = TEXT("false");
}

inline void Settings::SetString(const generic_string& section, const generic_string& key, const generic_string& value)
{
	(*iniFile)[section.c_str()][key.c_str()] = value.c_str();
}

#pragma endregion Internal INI processing