/** @file LexerCatalogue.cxx
 ** A simple Lexer catalogue
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "LexerCatalogue.h"

/*
* TASK: Include all your lexer modules here; update InstalledLexers
*/
#include "LexNWScript.h"

using std::size;
using namespace LexerInterface;

constexpr LexerDefinition InstalledLexers[] = {
	{"NWScript", TEXT("NWScript file"), SCLEX_NWSCRIPT, LexerNWScript::LexerFactoryNWScript},
	// {"NWScript NoCase", TEXT("NWScript Case Insensitive file"), SCLEX_NWSCRIPTNOCASE, LexerNWScript::LexerFactoryNWScriptInsensitive},
};


/*
* END OF TASK
*/


constexpr int InstalledLexersCount = (int)std::size(InstalledLexers);

int LexerCatalogue::GetLexerCount() {
	return InstalledLexersCount;
}

void LexerCatalogue::GetLexerName(unsigned int index, char* name, int buflength)
{
	if (index < InstalledLexersCount)
	{
		strncpy_s(name, buflength, InstalledLexers[index].lexerName, _TRUNCATE);
		name[buflength - 1] = L'\0';
	}
}

void LexerCatalogue::GetLexerStatusText(unsigned int index, WCHAR* desc, int buflength)
{
	if (index < InstalledLexersCount)
	{
		wcsncpy_s(desc, buflength, InstalledLexers[index].lexerStatusText, _TRUNCATE);
		desc[buflength - 1] = L'\0';
	}
}

MyLexerFactoryFunction LexerCatalogue::GetLexerFactory(unsigned int index)
{
	if (index < InstalledLexersCount)
		return InstalledLexers[index].ptrFactoryFunction;
	else
		return nullptr;
}

