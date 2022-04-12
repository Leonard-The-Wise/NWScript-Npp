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
	// Read or CREATE ini config.
	std::ignore = iniFilePath->read(*iniFile);

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
	installedEngineKnownObjects = GetBoolean(TEXT("Plugin Functions"), TEXT("installedEngineKnownObjects"));

	notepadRestartMode = static_cast<RestartMode>(GetNumber<int>(TEXT("Notepad Restart"), TEXT("notepadRestartMode")));
	notepadRestartFunction = static_cast<RestartFunctionHook>(GetNumber<int>(TEXT("Notepad Restart"), TEXT("notepadRestartFunction")));

	// Compiler settings
	compilerSettingsCreated = GetBoolean(TEXT("Compiler Settings"), TEXT("compilerSettingsCreated"));
	neverwinterInstallChoice = GetNumber<int>(TEXT("Compiler Settings"), TEXT("neverwinterInstallChoice"));
	neverwinterOneInstallDir = properDirNameW(GetString(TEXT("Compiler Settings"), TEXT("neverwinterOneInstallDir")));
	neverwinterTwoInstallDir = properDirNameW(GetString(TEXT("Compiler Settings"), TEXT("neverwinterTwoInstallDir")));
	ignoreInstallPaths = GetBoolean(TEXT("Compiler Settings"), TEXT("ignoreInstallPaths"));
	additionalIncludeDirs = GetString(TEXT("Compiler Settings"), TEXT("additionalIncludeDirs"));
	compilerFlags = GetNumber<int>(TEXT("Compiler Settings"), TEXT("compilerFlags"));
	optimizeScript = GetBoolean(TEXT("Compiler Settings"), TEXT("optimizeScript"));
	useNonBiowareExtenstions = GetBoolean(TEXT("Compiler Settings"), TEXT("useNonBiowareExtenstions"));
	generateSymbols = GetBoolean(TEXT("Compiler Settings"), TEXT("generateSymbols"));
	compileVersion = GetNumber<int>(TEXT("Compiler Settings"), TEXT("compileVersion"));
	useScriptPathToCompile = GetBoolean(TEXT("Compiler Settings"), TEXT("useScriptPathToCompile"));
	outputCompileDir = properDirNameW(GetString(TEXT("Compiler Settings"), TEXT("outputCompileDir")));

	// Batch Process
	startingBatchFolder = properDirNameW(GetString(TEXT("Batch Processing"), TEXT("startingBatchFolder")));
	fileFiltersCompile = GetString(TEXT("Batch Processing"), TEXT("fileFiltersCompile"));
	fileFiltersDisasm = GetString(TEXT("Batch Processing"), TEXT("fileFiltersDisasm"));
	batchCompileMode = GetNumber<int>(TEXT("Batch Processing"), TEXT("batchCompileMode"));
	recurseSubFolders = GetBoolean(TEXT("Batch Processing"), TEXT("recurseSubFolders"));
	continueCompileOnFail = GetBoolean(TEXT("Batch Processing"), TEXT("continueCompileOnFail"));
	useScriptPathToBatchCompile = GetBoolean(TEXT("Batch Processing"), TEXT("useScriptPathToBatchCompile"));
	batchOutputCompileDir = properDirNameW(GetString(TEXT("Batch Processing"), TEXT("batchOutputCompileDir")));

	// User's Preferences
	autoDisplayDisassembled = GetBoolean(TEXT("User's Preferences"), TEXT("autoDisplayDisassembled"));
	autoDisplayDebugSymbols = GetBoolean(TEXT("User's Preferences"), TEXT("autoDisplayDebugSymbols"));
	autoInstallDarkTheme = GetBoolean(TEXT("User's Preferences"), TEXT("autoInstallDarkTheme"));
	lastOpenedDir = properDirNameW(GetString(TEXT("User's Preferences"), TEXT("lastOpenedDir")));

	// Dark Theme auto-install support
	darkThemePreviouslyInstalled = GetBoolean(TEXT("Auto-Install"), TEXT("darkThemePreviouslyInstalled"));
	darkThemeInstallAttempt = GetBoolean(TEXT("Auto-Install"), TEXT("darkThemeInstallAttempt"));
	notepadVersion = GetString(TEXT("Auto-Install"), TEXT("notepadVersion"));

	// Plugin statistics
	compileAttempts = GetNumber<int>(TEXT("User Statistics"), TEXT("compileAttempts"));
	compileSuccesses = GetNumber<int>(TEXT("User Statistics"), TEXT("compileSuccesses"));
	compileFails = GetNumber<int>(TEXT("User Statistics"), TEXT("compileFails"));
	disassembledFiles = GetNumber<int>(TEXT("User Statistics"), TEXT("disassembledFiles"));
	engineStructs = GetNumber<int>(TEXT("User Statistics"), TEXT("engineStructs"));
	engineFunctionCount = GetNumber<int>(TEXT("User Statistics"), TEXT("engineFunctionCount"));
	engineConstants = GetNumber<int>(TEXT("User Statistics"), TEXT("engineConstants"));
	userFunctionCount = GetNumber<int>(TEXT("User Statistics"), TEXT("userFunctionCount"));
	userConstants = GetNumber<int>(TEXT("User Statistics"), TEXT("userConstants"));

	// Compiler window settings
	compilerWindowSelectedTab = GetNumber<int>(TEXT("Compiler Window"), TEXT("compilerWindowSelectedTab"));
	compilerWindowShowErrors = GetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowShowErrors"));
	compilerWindowShowWarnings = GetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowShowWarnings"));
	compilerWindowShowInfos = GetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowShowInfos"));
	compilerWindowConsoleWordWrap = GetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowConsoleWordWrap"));
	compilerWindowConsoleShowErrors = GetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowConsoleShowErrors"));
	compilerWindowConsoleShowWarnings = GetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowConsoleShowWarnings"));
	compilerWindowConsoleShowInfos = GetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowConsoleShowInfos"));


	// Sanity checks: avoid loading missing or corrupted data for compiled settings. Mark configurations invalid if inconsistency detected.
	if (!isValidDirectoryS(neverwinterOneInstallDir) && !isValidDirectoryS(neverwinterTwoInstallDir) && !ignoreInstallPaths)
	{
		neverwinterOneInstallDir = TEXT("");
		neverwinterTwoInstallDir = TEXT("");
		compilerSettingsCreated = false;
	}
	if (!isValidDirectoryS(neverwinterOneInstallDir) && neverwinterInstallChoice == 0 && !ignoreInstallPaths)
	{
		neverwinterOneInstallDir = TEXT("");
		compilerSettingsCreated = false;
	}
	if (!isValidDirectoryS(neverwinterTwoInstallDir) && neverwinterInstallChoice == 1 && !ignoreInstallPaths)
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

	if (!isValidDirectoryS(outputCompileDir) && !useScriptPathToCompile)
	{
		outputCompileDir = TEXT("");
		compilerSettingsCreated = false;
	}

	if (!isValidDirectoryS(lastOpenedDir))
		lastOpenedDir = TEXT("");

	// We aren't checking batch operations settings here, 
	// since the user will have to run the Dialog first to run a batch...

}

void Settings::Save()
{
	// Set this file as valid
	SetBoolean(TEXT("Plugin Startup"), TEXT("_bValidINI"), true);

	// Set all settings variables to INI here
	SetBoolean(TEXT("Plugin Functions"), TEXT("enableAutoIndentation"), enableAutoIndentation);
	SetBoolean(TEXT("Plugin Functions"), TEXT("autoIndentationWarningAccepted"), autoIndentationWarningAccepted);
	SetBoolean(TEXT("Plugin Functions"), TEXT("installedEngineKnownObjects"), installedEngineKnownObjects);

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
	SetNumber<int>(TEXT("Batch Processing"), TEXT("batchCompileMode"), batchCompileMode);
	SetBoolean(TEXT("Batch Processing"), TEXT("recurseSubFolders"), recurseSubFolders);
	SetBoolean(TEXT("Batch Processing"), TEXT("continueCompileOnFail"), continueCompileOnFail);
	SetBoolean(TEXT("Batch Processing"), TEXT("useScriptPathToBatchCompile"), useScriptPathToBatchCompile);
	SetString(TEXT("Batch Processing"), TEXT("batchOutputCompileDir"), batchOutputCompileDir);

	// User's Preferences
	SetBoolean(TEXT("User's Preferences"), TEXT("autoDisplayDisassembled"), autoDisplayDisassembled);
	SetBoolean(TEXT("User's Preferences"), TEXT("autoDisplayDebugSymbols"), autoDisplayDebugSymbols);
	SetBoolean(TEXT("User's Preferences"), TEXT("autoInstallDarkTheme"), autoInstallDarkTheme);
	SetString(TEXT("User's Preferences"), TEXT("lastOpenedDir"), lastOpenedDir);

	// Dark Theme auto-install support
	SetBoolean(TEXT("Auto-Install"), TEXT("darkThemePreviouslyInstalled"), darkThemePreviouslyInstalled);
	SetBoolean(TEXT("Auto-Install"), TEXT("darkThemeInstallAttempt"), darkThemeInstallAttempt);
	SetString(TEXT("Auto-Install"), TEXT("notepadVersion"), notepadVersion.wstring());

	// Plugin statistics
	SetNumber<int>(TEXT("User Statistics"), TEXT("compileAttempts"), compileAttempts);
	SetNumber<int>(TEXT("User Statistics"), TEXT("compileSuccesses"), compileSuccesses);
	SetNumber<int>(TEXT("User Statistics"), TEXT("compileFails"), compileFails);
	SetNumber<int>(TEXT("User Statistics"), TEXT("disassembledFiles"), disassembledFiles);
	SetNumber<int>(TEXT("User Statistics"), TEXT("engineStructs"), engineStructs);
	SetNumber<int>(TEXT("User Statistics"), TEXT("engineFunctionCount"), engineFunctionCount);
	SetNumber<int>(TEXT("User Statistics"), TEXT("engineConstants"), engineConstants);
	SetNumber<int>(TEXT("User Statistics"), TEXT("userFunctionCount"), userFunctionCount);
	SetNumber<int>(TEXT("User Statistics"), TEXT("userConstants"), userConstants);

	// Compiler window settings
	SetNumber<int>(TEXT("Compiler Window"), TEXT("compilerWindowSelectedTab"), compilerWindowSelectedTab);
	SetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowShowErrors"), compilerWindowShowErrors);
	SetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowShowWarnings"), compilerWindowShowWarnings);
	SetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowShowInfos"), compilerWindowShowInfos);
	SetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowConsoleWordWrap"), compilerWindowConsoleWordWrap);
	SetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowConsoleShowErrors"), compilerWindowConsoleShowErrors);
	SetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowConsoleShowWarnings"), compilerWindowConsoleShowWarnings);
	SetBoolean(TEXT("Compiler Window"), TEXT("compilerWindowConsoleShowInfos"), compilerWindowConsoleShowInfos);

	if (!iniFilePath->write(*iniFile))
		MessageBox(NULL,
			(TEXT("Could not save the .ini file: ") + iniFilePathName).c_str(),
			TEXT("NWScript-Npp shutdown"), MB_ICONERROR | MB_OK);
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
			results.push_back(properDirNameW(m[1]));
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