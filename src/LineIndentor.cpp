/** @file LineIndentor.cpp
 * Implements Base LineIndentor class.
 * Code extracted and adapted from Notepad++ Auto Indentation features
 * (original source: @Notepad_plus.cpp)
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "PluginMain.h"
#include "LineIndentor.h"

using namespace NWScriptPlugin;

// Returns TRUE if there is a conditional (if | else | for | while) expression on lineNumber
bool LineIndentor::isConditionExprLine(intptr_t lineNumber)
{
	if (lineNumber < 0 || lineNumber > pMsg.SendSciMessage<intptr_t>(SCI_GETLINECOUNT))
		return false;

	auto startPos = pMsg.SendSciMessage<intptr_t>(SCI_POSITIONFROMLINE, lineNumber);
	auto endPos = pMsg.SendSciMessage<intptr_t>(SCI_GETLINEENDPOSITION, lineNumber);
	pMsg.SendSciMessage<void>(SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
	pMsg.SendSciMessage<void>(SCI_SETTARGETRANGE, startPos, endPos);

	const char ifElseForWhileExpr[] = "((else[ \t]+)?if|for|while)[ \t]*[(].*[)][ \t]*|else[ \t]*";

	auto posFound = pMsg.SendSciMessage<intptr_t>(SCI_SEARCHINTARGET, strlen(ifElseForWhileExpr), reinterpret_cast<LPARAM>(ifElseForWhileExpr));
	if (posFound >= 0)
	{
		auto end = pMsg.SendSciMessage<intptr_t>(SCI_GETTARGETEND);
		if (end == endPos)
			return true;
	}

	return false;
}

// Look backwards to find targetSymbol
intptr_t LineIndentor::findMachedBracePos(size_t startPos, size_t endPos, char targetSymbol, char matchedSymbol)
{
	if (startPos == endPos)
		return -1;

	if (startPos > endPos) // backward
	{
		int balance = 0;
		for (intptr_t i = startPos; i >= static_cast<intptr_t>(endPos); --i)
		{
			char aChar = pMsg.SendSciMessage<char>(SCI_GETCHARAT, i);
			if (aChar == targetSymbol)
			{
				if (balance == 0)
					return i;
				--balance;
			}
			else if (aChar == matchedSymbol)
			{
				++balance;
			}
		}
	}

	return -1;
}

// Returns the range of current selected characters inside a Scintilla Text Editor window.
Sci_CharacterRange LineIndentor::getSelection()
{
	Sci_CharacterRange crange = {};
	crange.cpMin = pMsg.SendSciMessage<Sci_PositionCR>(SCI_GETSELECTIONSTART);
	crange.cpMax = pMsg.SendSciMessage<Sci_PositionCR>(SCI_GETSELECTIONEND);
	return crange;
};

// Sets the indentation size for a given line inside a Scintilla Text Editor window
void LineIndentor::setLineIndent(size_t line, size_t indent)
{
	Sci_CharacterRange crange = getSelection();
	size_t posBefore = pMsg.SendSciMessage<size_t>(SCI_GETLINEINDENTPOSITION, line);
	pMsg.SendSciMessage<>(SCI_SETLINEINDENTATION, line, indent);
	size_t posAfter = pMsg.SendSciMessage<size_t>(SCI_GETLINEINDENTPOSITION, line);
	long long posDifference = posAfter - posBefore;
	if (posAfter > posBefore)
	{
		// Move selection on
		if (crange.cpMin >= static_cast<Sci_PositionCR>(posBefore))
		{
			crange.cpMin += static_cast<Sci_PositionCR>(posDifference);
		}
		if (crange.cpMax >= static_cast<Sci_PositionCR>(posBefore))
		{
			crange.cpMax += static_cast<Sci_PositionCR>(posDifference);
		}
	}
	else if (posAfter < posBefore)
	{
		// Move selection back
		if (crange.cpMin >= static_cast<Sci_PositionCR>(posAfter))
		{
			if (crange.cpMin >= static_cast<Sci_PositionCR>(posBefore))
				crange.cpMin += static_cast<Sci_PositionCR>(posDifference);
			else
				crange.cpMin = static_cast<Sci_PositionCR>(posAfter);
		}

		if (crange.cpMax >= static_cast<Sci_PositionCR>(posAfter))
		{
			if (crange.cpMax >= static_cast<Sci_PositionCR>(posBefore))
				crange.cpMax += static_cast<Sci_PositionCR>(posDifference);
			else
				crange.cpMax = static_cast<Sci_PositionCR>(posAfter);
		}
	}
	pMsg.SendSciMessage<>(SCI_SETSEL, crange.cpMin, crange.cpMax);
}

// Gets the a given line Length inside a Scintilla Text Editor window
intptr_t LineIndentor::getLineLength(size_t line)
{
	return pMsg.SendSciMessage<intptr_t>(SCI_GETLINEENDPOSITION, line) - pMsg.SendSciMessage<intptr_t>(SCI_POSITIONFROMLINE, line);
}

// Gets the a given line Indention size inside a Scintilla Text Editor window
intptr_t LineIndentor::getLineIdent(size_t line)
{
	return pMsg.SendSciMessage<size_t>(SCI_GETLINEINDENTATION, line);
}

// Performs live Line Indentation using a given Character as an input
void LineIndentor::IndentLine(TCHAR ch)
{
	intptr_t eolMode = pMsg.SendSciMessage<intptr_t>(SCI_GETEOLMODE);
	intptr_t curLine = pMsg.SendSciMessage<intptr_t>(SCI_LINEFROMPOSITION, pMsg.SendSciMessage<intptr_t>(SCI_GETCURRENTPOS));
	intptr_t prevLine = curLine - 1;
	intptr_t indentAmountPrevLine = 0;
	intptr_t tabWidth = pMsg.SendSciMessage<intptr_t>(SCI_GETTABWIDTH);

	// Do not alter indentation if we were at the beginning of the line and we pressed Enter
	if ((((eolMode == SC_EOL_CRLF || eolMode == SC_EOL_LF) && ch == '\n') ||
		(eolMode == SC_EOL_CR && ch == '\r')) && prevLine >= 0 && getLineLength(prevLine) == 0)
		return;

	if (((eolMode == SC_EOL_CRLF || eolMode == SC_EOL_LF) && ch == '\n') ||
		(eolMode == SC_EOL_CR && ch == '\r'))
	{
		// Search the non-empty previous line

		while (prevLine >= 0 && getLineLength(prevLine) == 0)
			prevLine--;

		// Get previous line's Indent
		if (prevLine >= 0)
		{
			indentAmountPrevLine = getLineIdent(prevLine);
		}

		// get previous char from current line
		intptr_t prevPos = pMsg.SendSciMessage<intptr_t>(SCI_GETCURRENTPOS) - (eolMode == SC_EOL_CRLF ? 3 : 2);
		UCHAR prevChar = pMsg.SendSciMessage<UCHAR>(SCI_GETCHARAT, prevPos);
		auto curPos = pMsg.SendSciMessage<intptr_t>(SCI_GETCURRENTPOS);
		UCHAR nextChar = pMsg.SendSciMessage<UCHAR>(SCI_GETCHARAT, curPos);

		if (prevChar == '{')
		{
			if (nextChar == '}')
			{
				const char* eolChars;
				if (eolMode == SC_EOL_CRLF)
					eolChars = "\r\n";
				else if (eolMode == SC_EOL_LF)
					eolChars = "\n";
				else
					eolChars = "\r";

				pMsg.SendSciMessage<>(SCI_INSERTTEXT, pMsg.SendSciMessage<intptr_t>(SCI_GETCURRENTPOS), reinterpret_cast<LPARAM>(eolChars));
				setLineIndent(curLine + 1, indentAmountPrevLine);
			}
			setLineIndent(curLine, indentAmountPrevLine + tabWidth);
		}
		else if (nextChar == '{')
		{
			setLineIndent(curLine, indentAmountPrevLine);
		}
		else if (isConditionExprLine(prevLine))
		{
			setLineIndent(curLine, indentAmountPrevLine + tabWidth);
		}
		else
		{
			if (indentAmountPrevLine > 0)
			{
				if (prevLine > 0 && isConditionExprLine(prevLine - 1))
					setLineIndent(curLine, indentAmountPrevLine - tabWidth);
				else
					setLineIndent(curLine, indentAmountPrevLine);
			}
		}
	}
	else if (ch == '{')
	{
		// if no character in front of {, aligned with prev line's indentation
		auto startPos = pMsg.SendSciMessage<intptr_t>(SCI_POSITIONFROMLINE, curLine);
		LRESULT endPos = pMsg.SendSciMessage<intptr_t>(SCI_GETCURRENTPOS);

		for (LRESULT i = endPos - 2; i > 0 && i > startPos; --i)
		{
			UCHAR aChar = pMsg.SendSciMessage<UCHAR>(SCI_GETCHARAT, i);
			if (aChar != ' ' && aChar != '\t')
				return;
		}

		// Search the non-empty previous line
		while (prevLine >= 0 && getLineLength(prevLine) == 0)
			prevLine--;

		// Get previous line's Indent
		if (prevLine >= 0)
		{
			indentAmountPrevLine = getLineIdent(prevLine);

			auto startPos2 = pMsg.SendSciMessage<intptr_t>(SCI_POSITIONFROMLINE, prevLine);
			auto endPos2 = pMsg.SendSciMessage<intptr_t>(SCI_GETLINEENDPOSITION, prevLine);
			pMsg.SendSciMessage<intptr_t>(SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
			pMsg.SendSciMessage<intptr_t>(SCI_SETTARGETRANGE, startPos2, endPos2);

			const char braceExpr[] = "[ \t]*\\{.*";

			intptr_t posFound = pMsg.SendSciMessage<intptr_t>(SCI_SEARCHINTARGET, strlen(braceExpr), reinterpret_cast<LPARAM>(braceExpr));
			if (posFound >= 0)
			{
				auto end = pMsg.SendSciMessage<intptr_t>(SCI_GETTARGETEND);
				if (end == endPos2)
					indentAmountPrevLine += tabWidth;
			}
		}
		setLineIndent(curLine, indentAmountPrevLine);
	}
	else if (ch == '}')
	{
		// Look backward for the pair {
		intptr_t startPos = pMsg.SendSciMessage<intptr_t>(SCI_GETCURRENTPOS);
		if (startPos != 0)
			startPos -= 1;
		intptr_t posFound = findMachedBracePos(startPos - 1, 0, '{', '}');

		// if no { found, do nothing
		if (posFound == -1)
			return;

		// if { is in the same line, do nothing
		intptr_t matchedPairLine = pMsg.SendSciMessage<intptr_t>(SCI_LINEFROMPOSITION, posFound);
		if (matchedPairLine == curLine)
			return;

		// { is in another line, get its indentation
		indentAmountPrevLine = getLineIdent(matchedPairLine);

		// aligned } indent with {
		setLineIndent(curLine, indentAmountPrevLine);
	}
}
