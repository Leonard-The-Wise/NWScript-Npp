/** @file LexerCatalogue.h
 ** A simple Lexer catalogue
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <tchar.h>
#include <wtypes.h>

#include "ILexer.h"


using namespace Scintilla;

namespace LexerInterface {
	typedef ILexer5* (*MyLexerFactoryFunction)();

	struct LexerDefinition {
		const char lexerName[16] = {};
		const TCHAR lexerStatusText[32] = {};
		const int LexerID = 0;
		const MyLexerFactoryFunction ptrFactoryFunction = nullptr;
	};

	class LexerCatalogue {

	public:
		static int GetLexerCount();
		static void GetLexerName(unsigned int index, char* name, int buflength);
		static void GetLexerStatusText(unsigned int index, WCHAR* desc, int buflength);
		static MyLexerFactoryFunction GetLexerFactory(unsigned int index);
	};
}

// Shut up annoying Visual C++ warnings:
#ifdef _MSC_VER
#pragma warning(disable: 4244 4456 4457)
#endif

// Turn off shadow warnings for lexers as may be maintained by others
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wshadow"
#endif

// Clang doesn't like omitting braces in array initialization but they just add
// noise to LexicalClass arrays in lexers
#if defined(__clang__)
#pragma clang diagnostic ignored "-Wmissing-braces"
#endif
