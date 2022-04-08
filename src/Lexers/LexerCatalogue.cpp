/** @file LexerCatalogue.cpp
 ** A simple Lexer catalogue
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <stdexcept>
//#include <algorithm>
//#include <string>

#include "LexerCatalogue.h"

/*
* TASK: Include all your lexer modules here; update InstalledLexers
*/
#include "LexNWScript.h"

using std::size;
using namespace LexerInterface;

constexpr static LexerDefinition InstalledLexers[] = {
	{"NWScript", TEXT("NWScript file"), SCLEX_NWSCRIPT, LexerNWScript::LexerFactoryNWScript, ExternalLexerAutoIndentMode::C_Like},
	// {"NWScript NoCase", TEXT("NWScript Case Insensitive file"), SCLEX_NWSCRIPTNOCASE, LexerNWScript::LexerFactoryNWScriptInsensitive, ExternalLexerAutoIndentMode::Extended},
};


/*
* END OF TASK
*/

constexpr const int InstalledLexersCount = (int)std::size(InstalledLexers);

// To create different lexers, add other classes on this function.
ILexer5* LexerCatalogue::CreateLexer(const char* name)
{
	std::string _name = name;
	if (_name == InstalledLexers[0].lexerName)   
		return new LexerNWScript(true); // The only supported option for this plugin.
	else
		return nullptr;
}

int LexerCatalogue::GetLexerCount() noexcept {
	return InstalledLexersCount;
}

void LexerCatalogue::GetLexerName(unsigned int index, char* name, int buflength)
{
	if (index < InstalledLexersCount)
	{
		strncpy_s(name, buflength, InstalledLexers[index].lexerName, _TRUNCATE);
		name[buflength - 1] = L'\0';
	}
	else
		throw std::out_of_range("index out of bounds");
}

std::string LexerCatalogue::GetLexerName(unsigned int index, bool toLower)
{
	if (index < InstalledLexersCount)
	{
		std::string retVal = InstalledLexers[index].lexerName;
		if (toLower)
			std::transform(retVal.begin(), retVal.end(), retVal.begin(), ::tolower);

		return retVal;
	}
	else
		throw std::out_of_range("index out of bounds");
}

void LexerCatalogue::GetLexerStatusText(unsigned int index, WCHAR* desc, int buflength)
{
	if (index < InstalledLexersCount)
	{
		wcsncpy_s(desc, buflength, InstalledLexers[index].lexerStatusText, _TRUNCATE);
		desc[buflength - 1] = L'\0';
	}
	else
		throw std::out_of_range("index out of bounds");

}

MyLexerFactoryFunction LexerCatalogue::GetLexerFactory(unsigned int index)
{
	if (index < InstalledLexersCount)
		return InstalledLexers[index].ptrFactoryFunction;
	else
		throw std::out_of_range("index out of bounds");
}

ExternalLexerAutoIndentMode LexerCatalogue::GetLexerIndentType(unsigned int index)
{
	if (index < InstalledLexersCount)
		return InstalledLexers[index].langAutoIndent;
	else
		throw std::out_of_range("index out of bounds");
}


