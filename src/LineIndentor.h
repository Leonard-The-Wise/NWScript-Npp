/** @file LineIndentor.h
 * Overridable LineIndentor class.
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

namespace NWScriptPlugin {

	// The Plugin's base custom Line Indentor for a given Scintilla Text Editor window.	
	class LineIndentor
	{
	public:
		void SetMessenger(PluginMessenger& _pMsg) {
			pMsg = _pMsg;
		}

		// Performs live Line Indentation into the Scintilla Text Editor window using the given Character as an input.
		virtual void IndentLine(TCHAR ch);

	private:
		PluginMessenger pMsg;

		// Returns the range of current selected characters inside a Scintilla Text Editor window.
		Sci_CharacterRange getSelection();
		// Returns TRUE if there is a conditional (if | else | for | while) expression on lineNumber inside a Scintilla Text Editor window.
		bool isConditionExprLine(intptr_t lineNumber);
		// Returns the current position to a paired matchedSymbol for a given targetSymbol. Example: { [ ( ) ] } inside a Scintilla Text Editor window.
		intptr_t findMachedBracePos(size_t startPos, size_t endPos, char targetSymbol, char matchedSymbol);
		// Sets the indentation size for a given line inside a Scintilla Text Editor window
		void setLineIndent(size_t line, size_t indent);
		// Gets the a given line Length inside a Scintilla Text Editor window
		intptr_t getLineLength(size_t line);
		// Gets the a given line Indention size inside a Scintilla Text Editor window
		intptr_t getLineIdent(size_t line);
	};

}