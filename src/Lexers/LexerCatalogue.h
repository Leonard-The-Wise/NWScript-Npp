/** @file LexerCatalogue.h
 ** A simple Lexer catalogue
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <tchar.h>
#include <wtypes.h>

#include "ILexer.h"
#include "Notepad_plus_msgs.h"

#define MAX_LEXERNAMELENGTH 16
#define MAX_LEXERDESCLENGHT 32

using namespace Scintilla;

namespace LexerInterface {
	typedef ILexer5* (*MyLexerFactoryFunction)();

	struct LexerDefinition final {
		const char lexerName[MAX_LEXERNAMELENGTH] = {};
		const TCHAR lexerStatusText[MAX_LEXERDESCLENGHT] = {};
		const int LexerID = 0;
		const MyLexerFactoryFunction ptrFactoryFunction = nullptr;
		const ExternalLexerAutoIndentMode langAutoIndent = ExternalLexerAutoIndentMode::Standard;
	};

	class LexerCatalogue final {

	public:
		static int GetLexerCount() noexcept;
		static void GetLexerName(unsigned int index, char* name, int buflength);
		static const char* GetLexerName(unsigned int index);
		static void GetLexerStatusText(unsigned int index, WCHAR* desc, int buflength);
		static ExternalLexerAutoIndentMode GetLexerIndentType(unsigned int index);
		static MyLexerFactoryFunction GetLexerFactory(unsigned int index);
	};
}

