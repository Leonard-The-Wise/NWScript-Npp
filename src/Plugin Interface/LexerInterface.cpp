// Lexer Interface module
/** @file LexerInterface.cpp
 **
 ** This is where the Interface between Notepad++ and all avaliable Lexers is made.
 ** 
 ** Refactored file for better understanding by Leonardo Silva (2022) 
 **/
 // Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"

#include "DllMain.h"
#include "LexerInterface.h"

using namespace LexerInterface;

extern "C" DLLAPI inline int WINAPI GetLexerCount()
{
	return LexerCatalogue::GetLexerCount();
}

extern "C" DLLAPI inline void WINAPI GetLexerName(unsigned int index, char* name, int buflength)
{
	LexerCatalogue::GetLexerName(index, name, buflength);
}

extern "C" DLLAPI inline void WINAPI GetLexerStatusText(unsigned int index, WCHAR* desc, int buflength)
{
	LexerCatalogue::GetLexerStatusText(index, desc, buflength);
}

extern "C" DLLAPI inline MyLexerFactoryFunction WINAPI GetLexerFactory(unsigned int index)
{
	return LexerCatalogue::GetLexerFactory(index);
}
