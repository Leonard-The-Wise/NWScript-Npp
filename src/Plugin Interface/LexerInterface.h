// Lexer Interface module
/** @file LexerInterface.h
 **
 ** This is where the Interface between Notepad++ and all avaliable Lexers is made.
 **
 ** Refactored for better understanding by Leonardo Silva (2022)
 **
 **/
 // Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <string.h>
#include <wtypes.h>
#include "LexerCatalogue.h"

using namespace LexerInterface;

// This is everything you must export to Notepad++
extern "C" DLLAPI inline int WINAPI GetLexerCount();

extern "C" DLLAPI inline void WINAPI GetLexerName(unsigned int index, char* name, int buflength);

extern "C" DLLAPI inline void WINAPI GetLexerStatusText(unsigned int index, WCHAR * desc, int buflength);

extern "C" DLLAPI inline MyLexerFactoryFunction WINAPI GetLexerFactory(unsigned int index);

