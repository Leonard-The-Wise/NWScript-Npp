/** @file PluginMain.cpp
 * Controls the Plugin settings
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <algorithm>
#include <cctype>
#include <string>
#include <tchar.h>
#include <sstream>
#include <iomanip>

#include "Settings.h"

using namespace NWScriptPlugin;

void Settings::Load()
{
	iniFilePath->read(*iniFile);

	// TODO: Load all settings variables from INI here
	bEnableAutoIndentation = GetBoolean(TEXT("Plugin Functions"), TEXT("bEnableAutoIndentation"));
	bAutoIndentationWarningAccepted = GetBoolean(TEXT("Plugin Functions"), TEXT("bAutoIndentationWarningAccepted"));
	iNotepadRestartMode = static_cast<RestartMode>(GetNumber<int>(TEXT("Notepad Restart"), TEXT("iNotepadRestartMode")));
	iNotepadRestartFunction = static_cast<RestartFunctionHook>(GetNumber<int>(TEXT("Notepad Restart"), TEXT("iNotepadRestartFunction")));
}

void Settings::Save()
{
	// TODO: Set all settings variables to INI here
	SetBoolean(TEXT("Plugin Functions"), TEXT("bEnableAutoIndentation"), bEnableAutoIndentation);
	SetBoolean(TEXT("Plugin Functions"), TEXT("bAutoIndentationWarningAccepted"), bAutoIndentationWarningAccepted);
	SetNumber<int>(TEXT("Notepad Restart"), TEXT("iNotepadRestartMode"), static_cast<int>(iNotepadRestartMode));
	SetNumber<int>(TEXT("Notepad Restart"), TEXT("iNotepadRestartFunction"), static_cast<int>(iNotepadRestartFunction));

	iniFilePath->write(*iniFile);
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