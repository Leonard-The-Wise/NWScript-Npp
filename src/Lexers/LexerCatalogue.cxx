/** @file LexerCatalogue.cxx
 ** A simple Lexer catalogue
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <stdexcept>
#include "LexerCatalogue.h"

/*
* TASK: Include all your lexer modules here; update InstalledLexers
*/
#include "LexNWScript.h"

using std::size;
using namespace LexerInterface;

constexpr LexerDefinition InstalledLexers[] = {
	{"NWScript", TEXT("NWScript file"), SCLEX_NWSCRIPT, LexerNWScript::LexerFactoryNWScript, LangAutoIndentType::Extended},
	// {"NWScript NoCase", TEXT("NWScript Case Insensitive file"), SCLEX_NWSCRIPTNOCASE, LexerNWScript::LexerFactoryNWScriptInsensitive, LangAutoIndentType::Extended},
};


/*
* END OF TASK
*/

constexpr const int InstalledLexersCount = (int)std::size(InstalledLexers);

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

const char* LexerCatalogue::GetLexerName(unsigned int index)
{
	if (index < InstalledLexersCount)
		return InstalledLexers[index].lexerName;
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

LangAutoIndentType LexerCatalogue::GetLexerIndentType(unsigned int index)
{
	if (index < InstalledLexersCount)
		return InstalledLexers[index].langAutoIndent;
	else
		throw std::out_of_range("index out of bounds");
}


