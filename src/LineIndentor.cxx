/** @file LineIndentor.cxx
 * Code extracted and adapted from Notepad++ Auto Indentation features
 * (original source: @Notepad_plus.cpp)
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include "PluginMain.h"

using namespace NWScriptPlugin;

bool Plugin::LineIndentor::isConditionExprLine(intptr_t lineNumber)
{
	if (!pMsg)
		return false;

	if (lineNumber < 0 || lineNumber > Msg().SendSciMessage<intptr_t>(SCI_GETLINECOUNT))
		return false;

	auto startPos = Msg().SendSciMessage<intptr_t>(SCI_POSITIONFROMLINE, lineNumber);
	auto endPos = Msg().SendSciMessage<intptr_t>(SCI_GETLINEENDPOSITION, lineNumber);
	Msg().SendSciMessage<void>(SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
	Msg().SendSciMessage<void>(SCI_SETTARGETRANGE, startPos, endPos);

	const char ifElseForWhileExpr[] = "((else[ \t]+)?if|for|while)[ \t]*[(].*[)][ \t]*|else[ \t]*";

	auto posFound = Msg().SendSciMessage<intptr_t>(SCI_SEARCHINTARGET, strlen(ifElseForWhileExpr), reinterpret_cast<LPARAM>(ifElseForWhileExpr));
	if (posFound >= 0)
	{
		auto end = Msg().SendSciMessage<intptr_t>(SCI_GETTARGETEND);
		if (end == endPos)
			return true;
	}

	return false;
}

intptr_t Plugin::LineIndentor::findMachedBracePos(size_t startPos, size_t endPos, char targetSymbol, char matchedSymbol)
{
	if (startPos == endPos)
		return -1;

	if (startPos > endPos) // backward
	{
		int balance = 0;
		for (intptr_t i = startPos; i >= static_cast<intptr_t>(endPos); --i)
		{
			char aChar = Msg().SendSciMessage<char>(SCI_GETCHARAT, i);
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
	else // forward
	{
	}
	return -1;
}

Sci_CharacterRange Plugin::LineIndentor::getSelection()
{
	Sci_CharacterRange crange;
	crange.cpMin = Msg().SendSciMessage<Sci_PositionCR>(SCI_GETSELECTIONSTART);
	crange.cpMax = Msg().SendSciMessage<Sci_PositionCR>(SCI_GETSELECTIONEND);
	return crange;
};

void Plugin::LineIndentor::setLineIndent(size_t line, size_t indent)
{
	Sci_CharacterRange crange = getSelection();
	size_t posBefore = Msg().SendSciMessage<size_t>(SCI_GETLINEINDENTPOSITION, line);
	Msg().SendSciMessage<>(SCI_SETLINEINDENTATION, line, indent);
	size_t posAfter = Msg().SendSciMessage<size_t>(SCI_GETLINEINDENTPOSITION, line);
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
	Msg().SendSciMessage<>(SCI_SETSEL, crange.cpMin, crange.cpMax);
}

intptr_t Plugin::LineIndentor::getLineLength(size_t line)
{
	return Msg().SendSciMessage<intptr_t>(SCI_GETLINEENDPOSITION, line) - Msg().SendSciMessage<intptr_t>(SCI_POSITIONFROMLINE, line);
}

intptr_t Plugin::LineIndentor::getLineIdent(size_t line)
{
	return Msg().SendSciMessage<size_t>(SCI_GETLINEINDENTATION, line);
}

void Plugin::LineIndentor::SetMessenger(Messenger* SciMessenger)
{
	pMsg = SciMessenger;
}

void Plugin::LineIndentor::IndentLine(TCHAR ch)
{
	if (!pMsg)
		return;

	intptr_t eolMode = Msg().SendSciMessage<intptr_t>(SCI_GETEOLMODE);
	intptr_t curLine = Msg().SendSciMessage<intptr_t>(SCI_LINEFROMPOSITION, Msg().SendSciMessage<intptr_t>(SCI_GETCURRENTPOS));
	intptr_t prevLine = curLine - 1;
	intptr_t indentAmountPrevLine = 0;
	intptr_t tabWidth = Msg().SendSciMessage<intptr_t>(SCI_GETTABWIDTH);

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
		intptr_t prevPos = Msg().SendSciMessage<intptr_t>(SCI_GETCURRENTPOS) - (eolMode == SC_EOL_CRLF ? 3 : 2);
		UCHAR prevChar = Msg().SendSciMessage<UCHAR>(SCI_GETCHARAT, prevPos);
		auto curPos = Msg().SendSciMessage<intptr_t>(SCI_GETCURRENTPOS);
		UCHAR nextChar = Msg().SendSciMessage<UCHAR>(SCI_GETCHARAT, curPos);

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

				Msg().SendSciMessage<>(SCI_INSERTTEXT, Msg().SendSciMessage<intptr_t>(SCI_GETCURRENTPOS), reinterpret_cast<LPARAM>(eolChars));
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
		auto startPos = Msg().SendSciMessage<intptr_t>(SCI_POSITIONFROMLINE, curLine);
		LRESULT endPos = Msg().SendSciMessage<intptr_t>(SCI_GETCURRENTPOS);

		for (LRESULT i = endPos - 2; i > 0 && i > startPos; --i)
		{
			UCHAR aChar = Msg().SendSciMessage<UCHAR>(SCI_GETCHARAT, i);
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

			auto startPos2 = Msg().SendSciMessage<intptr_t>(SCI_POSITIONFROMLINE, prevLine);
			auto endPos2 = Msg().SendSciMessage<intptr_t>(SCI_GETLINEENDPOSITION, prevLine);
			Msg().SendSciMessage<intptr_t>(SCI_SETSEARCHFLAGS, SCFIND_REGEXP | SCFIND_POSIX);
			Msg().SendSciMessage<intptr_t>(SCI_SETTARGETRANGE, startPos2, endPos2);

			const char braceExpr[] = "[ \t]*\\{.*";

			intptr_t posFound = Msg().SendSciMessage<intptr_t>(SCI_SEARCHINTARGET, strlen(braceExpr), reinterpret_cast<LPARAM>(braceExpr));
			if (posFound >= 0)
			{
				auto end = Msg().SendSciMessage<intptr_t>(SCI_GETTARGETEND);
				if (end == endPos2)
					indentAmountPrevLine += tabWidth;
			}
		}
		setLineIndent(curLine, indentAmountPrevLine);
	}
	else if (ch == '}')
	{
		// Look backward for the pair {
		intptr_t startPos = Msg().SendSciMessage<intptr_t>(SCI_GETCURRENTPOS);
		if (startPos != 0)
			startPos -= 1;
		intptr_t posFound = findMachedBracePos(startPos - 1, 0, '{', '}');

		// if no { found, do nothing
		if (posFound == -1)
			return;

		// if { is in the same line, do nothing
		intptr_t matchedPairLine = Msg().SendSciMessage<intptr_t>(SCI_LINEFROMPOSITION, posFound);
		if (matchedPairLine == curLine)
			return;

		// { is in another line, get its indentation
		indentAmountPrevLine = getLineIdent(matchedPairLine);

		// aligned } indent with {
		setLineIndent(curLine, indentAmountPrevLine);
	}
}
