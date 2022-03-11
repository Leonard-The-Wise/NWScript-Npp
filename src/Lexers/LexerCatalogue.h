/** @file LexerCatalogue.h
 ** A simple Lexer catalogue
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <string>
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
		// ILexer5 interface implementing function. Must not be changed.
		static int GetLexerCount() noexcept;
		// ILexer5 interface implementing function. Must not be changed.
		static void GetLexerName(unsigned int index, char* name, int buflength);
		// Override for GetLexerName. Supports returning lowercase version.
		static std::string GetLexerName(unsigned int index, bool toLower = false);
		// ILexer5 interface implementing function. Must not be changed.
		static void GetLexerStatusText(unsigned int index, WCHAR* desc, int buflength);
		// Gets auto-indent mode for lexer.
		static ExternalLexerAutoIndentMode GetLexerIndentType(unsigned int index);
		// ILexer5 interface implementing function. Must not be changed.
		static MyLexerFactoryFunction GetLexerFactory(unsigned int index);
	};
}

