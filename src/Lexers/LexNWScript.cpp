//:://////////////////////////////////////////////////////////////////////////////////////////////////////
/** NWScript custom Lexer
 ** @file LexNWScript.cpp
 ** previous filename: LexCPP.cpp
 **
 ** Further folding features and configuration properties added by "Udo Lechner" <dlchnr(at)gmx(dot)net>
 ** Added extensions of NWScript language, changed name and class by "Leonardo Silva"
 **/
 // Copyright 1998-2005 by Neil Hodgson <neilh@scintilla.org>
 // The License.txt file describes the conditions under which this software may be distributed.
 //:://///////////////////////////////////////////////////////////////////////////////////////////////////
 //:: Patched by Leonardo Silva
 //:: File patched in Feb-2022 to adapt lexer for NWScript language
 //:://///////////////////////////////////////////////////////////////////////////////////////////////////

#include "LexNWScript.h"

/*
* 
* Implementations of LexNWScript class
*
*/

Sci_Position SCI_METHOD LexerNWScript::PropertySet(const char *key, const char *val) {
	if (osNWScript.PropertySet(&options, key, val)) {
		if (strcmp(key, "lexer.nwscript.allow.dollars") == 0) {
			setWord = CharacterSet(CharacterSet::setAlphaNum, "._", 0x80, true);
			if (options.identifiersAllowDollars) {
				setWord.Add('$');
			}
		}
		return 0;
	}
	return -1;
}

const char * SCI_METHOD LexerNWScript::PropertyGet(const char *key) {
	return osNWScript.PropertyGet(key);
}

Sci_Position SCI_METHOD LexerNWScript::WordListSet(int n, const char *wl) {
	WordList *wordListN = nullptr;

	// All keywordClass(es) from LexNWScript.xml
	switch (n) {
	case 0:
		wordListN = &keywordsInstructions;      // instre1
		break;
//	case 1:
//		wordListN = &keywordsInstr2;            // instre2 - this list is unused by this lexer. Kept from original file
//		break;
	case 2:
		wordListN = &keywordsCommonTypes;       // type1
		break;
	case 3:
		wordListN = &keywordsEngineTypes;       // type2
		break;
	case 4:
		wordListN = &keywordsObjectTypes;       // type3
		break;
	case 5:
		wordListN = &keywordsEngineConstants;   // type4
		break;
	case 6:
		wordListN = &keywordsUserConstants;     // type5
		break;
	case 7:
		wordListN = &keywordsEngineFunctions;   // type6
		break;
	case 8:
		wordListN = &keywordsUserFunctions;     // type7
		break;
	}
	Sci_Position firstModification = -1;
	if (wordListN) {
		WordList wlNew;
		wlNew.Set(wl);
		if (*wordListN != wlNew) {
			wordListN->Set(wl);
			firstModification = 0; 
		}
	}
	return firstModification;
}

void SCI_METHOD LexerNWScript::Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {
	LexAccessor styler(pAccess);

	CharacterSet setOKBeforeRE(CharacterSet::setNone, "([{=,:;!%^&*|?~+-");
	CharacterSet setCouldBePostOp(CharacterSet::setNone, "+-");

	CharacterSet setDoxygen(CharacterSet::setAlpha, "$@\\&<>#{}[]");

	setWordStart = CharacterSet(CharacterSet::setAlpha, "_", 0x80, true);

	CharacterSet setInvalidRawFirst(CharacterSet::setNone, " )\\\t\v\f\n");

	if (options.identifiersAllowDollars) {
		setWordStart.Add('$');
	}

	int chPrevNonWhite = ' ';
	int visibleChars = 0;
	bool lastWordWasUUID = false;
	int styleBeforeDCKeyword = SCE_C_DEFAULT;
	int styleBeforeTaskMarker = SCE_C_DEFAULT;
	bool continuationLine = false;
	bool isIncludePreprocessor = false;
	bool isStringInPreprocessor = false;
	bool inRERange = false;
	bool seenDocKeyBrace = false;

	Sci_Position lineCurrent = styler.GetLine(startPos);
	if ((MaskActive(initStyle) == SCE_C_PREPROCESSOR) ||
      (MaskActive(initStyle) == SCE_C_COMMENTLINE) ||
      (MaskActive(initStyle) == SCE_C_COMMENTLINEDOC)) {
		// Set continuationLine if last character of previous line is '\'
		if (lineCurrent > 0) {
			const Sci_Position endLinePrevious = styler.LineEnd(lineCurrent - 1);
			if (endLinePrevious > 0) {
				continuationLine = styler.SafeGetCharAt(endLinePrevious-1) == '\\';
			}
		}
	}

	// look back to set chPrevNonWhite properly for better regex colouring
	if (startPos > 0) {
		Sci_Position back = startPos;
		while (--back && IsSpaceEquiv(MaskActive(styler.StyleAt(back))))
			;
		if (MaskActive(styler.StyleAt(back)) == SCE_C_OPERATOR) {
			chPrevNonWhite = styler.SafeGetCharAt(back);
		}
	}

	StyleContext sc(startPos, length, initStyle, styler);
	LinePPState preproc = vlls.ForLine(lineCurrent);

	bool definitionsChanged = false;

	// Truncate ppDefineHistory before current line

	if (!options.updatePreprocessor)
		ppDefineHistory.clear();

	std::vector<PPDefinition>::iterator itInvalid = std::find_if(ppDefineHistory.begin(), ppDefineHistory.end(),
		[lineCurrent](const PPDefinition &p) noexcept { return p.line >= lineCurrent; });
	if (itInvalid != ppDefineHistory.end()) {
		ppDefineHistory.erase(itInvalid, ppDefineHistory.end());
		definitionsChanged = true;
	}

	SymbolTable preprocessorDefinitions = preprocessorDefinitionsStart;
	for (const PPDefinition &ppDef : ppDefineHistory) {
		if (ppDef.isUndef)
			preprocessorDefinitions.erase(ppDef.key);
		else
			preprocessorDefinitions[ppDef.key] = SymbolValue(ppDef.value, ppDef.arguments);
	}

	std::string rawStringTerminator = rawStringTerminators.ValueAt(lineCurrent-1);
	SparseState<std::string> rawSTNew(lineCurrent);

	int activitySet = preproc.ActiveState();

	const WordClassifier &classifierIdentifiers = subStyles.Classifier(SCE_C_IDENTIFIER);
	const WordClassifier &classifierDocKeyWords = subStyles.Classifier(SCE_C_COMMENTDOCKEYWORD);

	Sci_PositionU lineEndNext = styler.LineEnd(lineCurrent);

	for (; sc.More();) {

		if (sc.atLineStart) {
			// Using MaskActive() is not needed in the following statement.
			// Inside inactive preprocessor declaration, state will be reset anyway at the end of this block.
			if ((sc.state == SCE_C_STRING) || (sc.state == SCE_C_CHARACTER)) {
				// Prevent SCE_C_STRINGEOL from leaking back to previous line which
				// ends with a line continuation by locking in the state up to this position.
				sc.SetState(sc.state);
			}
			if ((MaskActive(sc.state) == SCE_C_PREPROCESSOR) && (!continuationLine)) {
				sc.SetState(SCE_C_DEFAULT|activitySet);
			}
			// Reset states to beginning of colourise so no surprises
			// if different sets of lines lexed.
			visibleChars = 0;
			lastWordWasUUID = false;
			isIncludePreprocessor = false;
			inRERange = false;
			if (preproc.IsInactive()) {
				activitySet = inactiveFlag;
				sc.SetState(sc.state | activitySet);
			}
		}

		if (sc.atLineEnd) {
			lineCurrent++;
			lineEndNext = styler.LineEnd(lineCurrent);
			vlls.Add(lineCurrent, preproc);
			if (rawStringTerminator != "") {
				rawSTNew.Set(lineCurrent-1, rawStringTerminator);
			}
		}

		// Handle line continuation generically.
		if (sc.ch == '\\') {
			if ((sc.currentPos+1) >= lineEndNext) {
				lineCurrent++;
				lineEndNext = styler.LineEnd(lineCurrent);
				vlls.Add(lineCurrent, preproc);
				if (rawStringTerminator != "") {
					rawSTNew.Set(lineCurrent-1, rawStringTerminator);
				}
				sc.Forward();
				if (sc.ch == '\r' && sc.chNext == '\n') {
					// Even in UTF-8, \r and \n are separate
					sc.Forward();
				}
				continuationLine = true;
				sc.Forward();
				continue;
			}
		}

		const bool atLineEndBeforeSwitch = sc.atLineEnd;

		// Determine if the current state should terminate.
		switch (MaskActive(sc.state)) {
			case SCE_C_OPERATOR:
				sc.SetState(SCE_C_DEFAULT|activitySet);
				break;
			case SCE_C_NUMBER:
				// We accept almost anything because of hex. and number suffixes
				if (sc.ch == '_') {
					sc.ChangeState(SCE_C_USERLITERAL|activitySet);
				} else if (!(setWord.Contains(sc.ch)
				   || (sc.ch == '\'')
				   || ((sc.ch == '+' || sc.ch == '-') && (sc.chPrev == 'e' || sc.chPrev == 'E' ||
				                                          sc.chPrev == 'p' || sc.chPrev == 'P')))) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_USERLITERAL:
				if (!(setWord.Contains(sc.ch)))
					sc.SetState(SCE_C_DEFAULT|activitySet);
				break;
			case SCE_C_IDENTIFIER:
				if (sc.atLineStart || sc.atLineEnd || !setWord.Contains(sc.ch) || (sc.ch == '.')) {
					char s[1000];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (keywordsInstructions.InList(s)) {    // instre1 stylers.xml keywordClass from notepad++
						lastWordWasUUID = strcmp(s, "uuid") == 0;
						sc.ChangeState(SCE_C_WORD|activitySet);
					} else if (keywordsInstr2.InList(s) || keywordsCommonTypes.InList(s)) { // notepad's instre2 or type1 (shared between langs)
						sc.ChangeState(SCE_C_WORD2|activitySet);
					} else if (keywordsEngineTypes.InList(s)) {				// type2
						sc.ChangeState(SCE_C_ENGINETYPE|activitySet);
					} else if (keywordsObjectTypes.InList(s)) {				// type3
						sc.ChangeState(SCE_C_OBJECTTYPE|activitySet);
					} else if (keywordsEngineConstants.InList(s)) {			// type4
						sc.ChangeState(SCE_C_ENGINECONSTANT|activitySet);
					} else if (keywordsUserConstants.InList(s)) {			// type5
						sc.ChangeState(SCE_C_USERCONSTANT|activitySet);
					} else if (keywordsEngineFunctions.InList(s)) {			// type6
						sc.ChangeState(SCE_C_ENGINEFUNCTION|activitySet);
					} else if (keywordsUserFunctions.InList(s)) {			// type7
						sc.ChangeState(SCE_C_USERFUNCTION|activitySet);
					} else {
						int subStyle = classifierIdentifiers.ValueFor(s);
						if (subStyle >= 0) {
							sc.ChangeState(subStyle|activitySet);
						}
					}
					const bool literalString = sc.ch == '\"';
					if (literalString || sc.ch == '\'') {
						size_t lenS = strlen(s);
						const bool raw = literalString && sc.chPrev == 'R' && !setInvalidRawFirst.Contains(sc.chNext);
						if (raw)
							s[lenS--] = '\0';
						const bool valid =
							(lenS == 0) ||
							((lenS == 1) && ((s[0] == 'L') || (s[0] == 'u') || (s[0] == 'U'))) ||
							((lenS == 2) && literalString && (s[0] == 'u') && (s[1] == '8'));
						if (valid) {
							if (literalString) {
								if (raw) {
									// Set the style of the string prefix to SCE_C_STRINGRAW but then change to
									// SCE_C_DEFAULT as that allows the raw string start code to run.
									sc.ChangeState(SCE_C_STRINGRAW|activitySet);
									sc.SetState(SCE_C_DEFAULT|activitySet);
								} else {
									sc.ChangeState(SCE_C_STRING|activitySet);
								}
							} else {
								sc.ChangeState(SCE_C_CHARACTER|activitySet);
							}
						} else {
							sc.SetState(SCE_C_DEFAULT | activitySet);
						}
					} else {
						sc.SetState(SCE_C_DEFAULT|activitySet);
					}
				}
				break;
			case SCE_C_PREPROCESSOR:
				if (options.stylingWithinPreprocessor) {
					if (IsASpace(sc.ch) || (sc.ch == '(')) {
						sc.SetState(SCE_C_DEFAULT|activitySet);
					}
				} else if (isStringInPreprocessor && (sc.Match('>') || sc.Match('\"') || sc.atLineEnd)) {
					isStringInPreprocessor = false;
				} else if (!isStringInPreprocessor) {
					if ((isIncludePreprocessor && sc.Match('<')) || sc.Match('\"')) {
						isStringInPreprocessor = true;
					} else if (sc.Match('/', '*')) {
						if (sc.Match("/**") || sc.Match("/*!")) {
							sc.SetState(SCE_C_PREPROCESSORCOMMENTDOC|activitySet);
						} else {
							sc.SetState(SCE_C_PREPROCESSORCOMMENT|activitySet);
						}
						sc.Forward();	// Eat the *
					} else if (sc.Match('/', '/')) {
						sc.SetState(SCE_C_DEFAULT|activitySet);
					}
				}
				break;
			case SCE_C_PREPROCESSORCOMMENT:
			case SCE_C_PREPROCESSORCOMMENTDOC:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_C_PREPROCESSOR|activitySet);
					continue;	// Without advancing in case of '\'.
				}
				break;
			case SCE_C_COMMENT:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
				} else {
					styleBeforeTaskMarker = SCE_C_COMMENT;
					highlightTaskMarker(sc, styler, activitySet, markerList, caseSensitive);
				}
				break;
			case SCE_C_COMMENTDOC:
				if (sc.Match('*', '/')) {
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '*') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_C_COMMENTDOC;
						sc.SetState(SCE_C_COMMENTDOCKEYWORD|activitySet);
					}
				}
				break;
			case SCE_C_COMMENTLINE:
				if (sc.atLineStart && !continuationLine) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else {
					styleBeforeTaskMarker = SCE_C_COMMENTLINE;
					highlightTaskMarker(sc, styler, activitySet, markerList, caseSensitive);
				}
				break;
			case SCE_C_COMMENTLINEDOC:
				if (sc.atLineStart && !continuationLine) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.ch == '@' || sc.ch == '\\') { // JavaDoc and Doxygen support
					// Verify that we have the conditions to mark a comment-doc-keyword
					if ((IsASpace(sc.chPrev) || sc.chPrev == '/' || sc.chPrev == '!') && (!IsASpace(sc.chNext))) {
						styleBeforeDCKeyword = SCE_C_COMMENTLINEDOC;
						sc.SetState(SCE_C_COMMENTDOCKEYWORD|activitySet);
					}
				}
				break;
			case SCE_C_COMMENTDOCKEYWORD:
				if ((styleBeforeDCKeyword == SCE_C_COMMENTDOC) && sc.Match('*', '/')) {
					sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR);
					sc.Forward();
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
					seenDocKeyBrace = false;
				} else if (sc.ch == '[' || sc.ch == '{') {
					seenDocKeyBrace = true;
				} else if (!setDoxygen.Contains(sc.ch)
				           && !(seenDocKeyBrace && (sc.ch == ',' || sc.ch == '.'))) {
					char s[100];
					if (caseSensitive) {
						sc.GetCurrent(s, sizeof(s));
					} else {
						sc.GetCurrentLowered(s, sizeof(s));
					}
					if (!(IsASpace(sc.ch) || (sc.ch == 0))) {
						sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR|activitySet);
					} else if (!keywordsCommonTypes.InList(s + 1)) {
						int subStyleCDKW = classifierDocKeyWords.ValueFor(s+1);
						if (subStyleCDKW >= 0) {
							sc.ChangeState(subStyleCDKW|activitySet);
						} else {
							sc.ChangeState(SCE_C_COMMENTDOCKEYWORDERROR|activitySet);
						}
					}
					sc.SetState(styleBeforeDCKeyword|activitySet);
					seenDocKeyBrace = false;
				}
				break;
			case SCE_C_STRING:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_C_STRINGEOL|activitySet);
				} else if (isIncludePreprocessor) {
					if (sc.ch == '>') {
						sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
						isIncludePreprocessor = false;
					}
				} else if (sc.ch == '\\') {
					if (options.escapeSequence) {
						sc.SetState(SCE_C_ESCAPESEQUENCE|activitySet);
						escapeSeq.resetEscapeState(sc.chNext);
					}
					sc.Forward(); // Skip all characters after the backslash
				} else if (sc.ch == '\"') {
					if (sc.chNext == '_') {
						sc.ChangeState(SCE_C_USERLITERAL|activitySet);
					} else {
						sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
					}
				}
				break;
			case SCE_C_ESCAPESEQUENCE:
				escapeSeq.consumeDigit();
				if (!escapeSeq.atEscapeEnd(sc.ch)) {
					break;
				}
				if (sc.ch == '"') {
					sc.SetState(SCE_C_STRING|activitySet);
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.ch == '\\') {
					escapeSeq.resetEscapeState(sc.chNext);
					sc.Forward();
				} else {
					sc.SetState(SCE_C_STRING|activitySet);
					if (sc.atLineEnd) {
						sc.ChangeState(SCE_C_STRINGEOL|activitySet);
					}
				}
				break;
			case SCE_C_HASHQUOTEDSTRING:
				if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\"') {
					sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_STRINGRAW:
				if (sc.Match(rawStringTerminator.c_str())) {
					for (size_t termPos=rawStringTerminator.size(); termPos; termPos--)
						sc.Forward();
					sc.SetState(SCE_C_DEFAULT|activitySet);
					rawStringTerminator = "";
				}
				break;
			case SCE_C_CHARACTER:
				if (sc.atLineEnd) {
					sc.ChangeState(SCE_C_STRINGEOL|activitySet);
				} else if (sc.ch == '\\') {
					if (sc.chNext == '\"' || sc.chNext == '\'' || sc.chNext == '\\') {
						sc.Forward();
					}
				} else if (sc.ch == '\'') {
					if (sc.chNext == '_') {
						sc.ChangeState(SCE_C_USERLITERAL|activitySet);
					} else {
						sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
					}
				}
				break;
			case SCE_C_REGEX:
				if (sc.atLineStart) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else if (!inRERange && sc.ch == '/') {
					sc.Forward();
					while (IsLowerCase(sc.ch))
						sc.Forward();    // gobble regex flags
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.ch == '\\' && ((sc.currentPos+1) < lineEndNext)) {
					// Gobble up the escaped character
					sc.Forward();
				} else if (sc.ch == '[') {
					inRERange = true;
				} else if (sc.ch == ']') {
					inRERange = false;
				}
				break;
			case SCE_C_STRINGEOL:
				if (sc.atLineStart) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_VERBATIM:
				if (options.verbatimStringsAllowEscapes && (sc.ch == '\\')) {
					sc.Forward(); // Skip all characters after the backslash
				} else if (sc.ch == '\"') {
					if (sc.chNext == '\"') {
						sc.Forward();
					} else {
						sc.ForwardSetState(SCE_C_DEFAULT|activitySet);
					}
				}
				break;
			case SCE_C_TRIPLEVERBATIM:
				if (sc.Match(R"(""")")) {
					while (sc.Match('"')) {
						sc.Forward();
					}
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_UUID:
				if (sc.atLineEnd || sc.ch == ')') {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				}
				break;
			case SCE_C_TASKMARKER:
				if (isoperator(sc.ch) || IsASpace(sc.ch)) {
					sc.SetState(styleBeforeTaskMarker|activitySet);
					styleBeforeTaskMarker = SCE_C_DEFAULT;
				}
		}

		if (sc.atLineEnd && !atLineEndBeforeSwitch) {
			// State exit processing consumed characters up to end of line.
			lineCurrent++;
			lineEndNext = styler.LineEnd(lineCurrent);
			vlls.Add(lineCurrent, preproc);
		}

		// Determine if a new state should be entered.
		if (MaskActive(sc.state) == SCE_C_DEFAULT) {
			if (sc.Match('@', '\"')) {
				sc.SetState(SCE_C_VERBATIM|activitySet);
				sc.Forward();
			} else if (options.triplequotedStrings && sc.Match(R"(""")")) {
				sc.SetState(SCE_C_TRIPLEVERBATIM|activitySet);
				sc.Forward(2);
			} else if (options.hashquotedStrings && sc.Match('#', '\"')) {
				sc.SetState(SCE_C_HASHQUOTEDSTRING|activitySet);
				sc.Forward();
			} else if (options.backQuotedStrings && sc.Match('`')) {
				sc.SetState(SCE_C_STRINGRAW|activitySet);
				rawStringTerminator = "`";
			} else if (IsADigit(sc.ch) || (sc.ch == '.' && IsADigit(sc.chNext))) {
				if (lastWordWasUUID) {
					sc.SetState(SCE_C_UUID|activitySet);
					lastWordWasUUID = false;
				} else {
					sc.SetState(SCE_C_NUMBER|activitySet);
				}
			} else if (!sc.atLineEnd && (setWordStart.Contains(sc.ch) || (sc.ch == '@'))) {
				if (lastWordWasUUID) {
					sc.SetState(SCE_C_UUID|activitySet);
					lastWordWasUUID = false;
				} else {
					sc.SetState(SCE_C_IDENTIFIER|activitySet);
				}
			} else if (sc.Match('/', '*')) {
				if (sc.Match("/**") || sc.Match("/*!")) {	// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_C_COMMENTDOC|activitySet);
				} else {
					sc.SetState(SCE_C_COMMENT|activitySet);
				}
				sc.Forward();	// Eat the * so it isn't used for the end of the comment
			} else if (sc.Match('/', '/')) {
				if ((sc.Match("///") && !sc.Match("////")) || sc.Match("//!"))
					// Support of Qt/Doxygen doc. style
					sc.SetState(SCE_C_COMMENTLINEDOC|activitySet);
				else
					sc.SetState(SCE_C_COMMENTLINE|activitySet);
			} else if (sc.ch == '/'
				   && (setOKBeforeRE.Contains(chPrevNonWhite)
				       || followsReturnKeyword(sc, styler))
				   && (!setCouldBePostOp.Contains(chPrevNonWhite)
				       || !FollowsPostfixOperator(sc, styler))) {
				sc.SetState(SCE_C_REGEX|activitySet);	// JavaScript's RegEx
				inRERange = false;
			} else if (sc.ch == '\"') {
				if (sc.chPrev == 'R') {
					styler.Flush();
					if (MaskActive(styler.StyleAt(sc.currentPos - 1)) == SCE_C_STRINGRAW) {
						sc.SetState(SCE_C_STRINGRAW|activitySet);
						rawStringTerminator = ")";
						for (Sci_Position termPos = sc.currentPos + 1;; termPos++) {
							const char chTerminator = styler.SafeGetCharAt(termPos, '(');
							if (chTerminator == '(')
								break;
							rawStringTerminator += chTerminator;
						}
						rawStringTerminator += '\"';
					} else {
						sc.SetState(SCE_C_STRING|activitySet);
					}
				} else {
					sc.SetState(SCE_C_STRING|activitySet);
				}
				isIncludePreprocessor = false;	// ensure that '>' won't end the string
			} else if (isIncludePreprocessor && sc.ch == '<') {
				sc.SetState(SCE_C_STRING|activitySet);
			} else if (sc.ch == '\'') {
				sc.SetState(SCE_C_CHARACTER|activitySet);
			} else if (sc.ch == '#' && visibleChars == 0) {
				// Preprocessor commands are alone on their line
				sc.SetState(SCE_C_PREPROCESSOR|activitySet);
				// Skip whitespace between # and preprocessor word
				do {
					sc.Forward();
				} while ((sc.ch == ' ' || sc.ch == '\t') && sc.More());
				if (sc.atLineEnd) {
					sc.SetState(SCE_C_DEFAULT|activitySet);
				} else if (sc.Match("include")) {
					isIncludePreprocessor = true;
				} else {
					if (options.trackPreprocessor) {
						// If #if is nested too deeply (>31 levels) the active/inactive appearance
						// will stop reflecting the code.
						if (sc.Match("ifdef") || sc.Match("ifndef")) {
							const bool isIfDef = sc.Match("ifdef");
							const int startRest = isIfDef ? 5 : 6;
							std::string restOfLine = GetRestOfLine(styler, sc.currentPos + startRest + 1, false);
							bool foundDef = preprocessorDefinitions.find(restOfLine) != preprocessorDefinitions.end();
							preproc.StartSection(isIfDef == foundDef);
						} else if (sc.Match("if")) {
							std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 2, true);
							const bool ifGood = EvaluateExpression(restOfLine, preprocessorDefinitions);
							preproc.StartSection(ifGood);
						} else if (sc.Match("else")) {
							// #else is shown as active if either preceding or following section is active
							// as that means that it contributed to the result.
							if (!preproc.CurrentIfTaken()) {
								// Inactive, may become active if parent scope active
								assert(sc.state == (SCE_C_PREPROCESSOR|inactiveFlag));
								preproc.InvertCurrentLevel();
								activitySet = preproc.ActiveState();
								// If following is active then show "else" as active
								if (!activitySet)
									sc.ChangeState(SCE_C_PREPROCESSOR);
							} else if (preproc.IsActive()) {
								// Active -> inactive
								assert(sc.state == SCE_C_PREPROCESSOR);
								preproc.InvertCurrentLevel();
								activitySet = preproc.ActiveState();
								// Continue to show "else" as active as it ends active section.
							}
						} else if (sc.Match("elif")) {
							// Ensure only one chosen out of #if .. #elif .. #elif .. #else .. #endif
							// #elif is shown as active if either preceding or following section is active
							// as that means that it contributed to the result.
							if (!preproc.CurrentIfTaken()) {
								// Inactive, if expression true then may become active if parent scope active
								assert(sc.state == (SCE_C_PREPROCESSOR|inactiveFlag));
								// Similar to #if
								std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 4, true);
								const bool ifGood = EvaluateExpression(restOfLine, preprocessorDefinitions);
								if (ifGood) {
									preproc.InvertCurrentLevel();
									activitySet = preproc.ActiveState();
									if (!activitySet)
										sc.ChangeState(SCE_C_PREPROCESSOR);
								}
							} else if (preproc.IsActive()) {
								// Active -> inactive
								assert(sc.state == SCE_C_PREPROCESSOR);
								preproc.InvertCurrentLevel();
								activitySet = preproc.ActiveState();
								// Continue to show "elif" as active as it ends active section.
							}
						} else if (sc.Match("endif")) {
							preproc.EndSection();
							activitySet = preproc.ActiveState();
							sc.ChangeState(SCE_C_PREPROCESSOR|activitySet);
						} else if (sc.Match("define")) {
							if (options.updatePreprocessor && preproc.IsActive()) {
								std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 6, true);
								size_t startName = 0;
								while ((startName < restOfLine.length()) && IsSpaceOrTab(restOfLine[startName]))
									startName++;
								size_t endName = startName;
								while ((endName < restOfLine.length()) && setWord.Contains(restOfLine[endName]))
									endName++;
								std::string key = restOfLine.substr(startName, endName-startName);
								if ((endName < restOfLine.length()) && (restOfLine.at(endName) == '(')) {
									// Macro
									size_t endArgs = endName;
									while ((endArgs < restOfLine.length()) && (restOfLine[endArgs] != ')'))
										endArgs++;
									std::string args = restOfLine.substr(endName + 1, endArgs - endName - 1);
									size_t startValue = endArgs+1;
									while ((startValue < restOfLine.length()) && IsSpaceOrTab(restOfLine[startValue]))
										startValue++;
									std::string value;
									if (startValue < restOfLine.length())
										value = restOfLine.substr(startValue);
									preprocessorDefinitions[key] = SymbolValue(value, args);
									ppDefineHistory.push_back(PPDefinition(lineCurrent, key, value, false, args));
									definitionsChanged = true;
								} else {
									// Value
									size_t startValue = endName;
									while ((startValue < restOfLine.length()) && IsSpaceOrTab(restOfLine[startValue]))
										startValue++;
									std::string value = restOfLine.substr(startValue);
									if (OnlySpaceOrTab(value))
										value = "1";	// No value defaults to 1
									preprocessorDefinitions[key] = value;
									ppDefineHistory.push_back(PPDefinition(lineCurrent, key, value));
									definitionsChanged = true;
								}
							}
						} else if (sc.Match("undef")) {
							if (options.updatePreprocessor && preproc.IsActive()) {
								const std::string restOfLine = GetRestOfLine(styler, sc.currentPos + 5, false);
								std::vector<std::string> tokens = Tokenize(restOfLine);
								if (tokens.size() >= 1) {
									const std::string key = tokens[0];
									preprocessorDefinitions.erase(key);
									ppDefineHistory.push_back(PPDefinition(lineCurrent, key, "", true));
									definitionsChanged = true;
								}
							}
						}
					}
				}
			} else if (isoperator(sc.ch)) {
				sc.SetState(SCE_C_OPERATOR|activitySet);
			}
		}

		if (!IsASpace(sc.ch) && !IsSpaceEquiv(MaskActive(sc.state))) {
			chPrevNonWhite = sc.ch;
			visibleChars++;
		}
		continuationLine = false;
		sc.Forward();
	}
	const bool rawStringsChanged = rawStringTerminators.Merge(rawSTNew, lineCurrent);
	if (definitionsChanged || rawStringsChanged)
		styler.ChangeLexerState(startPos, startPos + length);
	sc.Complete();
}

// Store both the current line's fold level and the next lines in the
// level store to make it easy to pick up with each increment
// and to make it possible to fiddle the current level for "} else {".

void SCI_METHOD LexerNWScript::Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument *pAccess) {

	if (!options.fold)
		return;

	LexAccessor styler(pAccess);

	const Sci_PositionU endPos = startPos + length;
	int visibleChars = 0;
	bool inLineComment = false;
	Sci_Position lineCurrent = styler.GetLine(startPos);
	int levelCurrent = SC_FOLDLEVELBASE;
	if (lineCurrent > 0)
		levelCurrent = styler.LevelAt(lineCurrent-1) >> 16;
	Sci_PositionU lineStartNext = styler.LineStart(lineCurrent+1);
	int levelMinCurrent = levelCurrent;
	int levelNext = levelCurrent;
	char chNext = styler[startPos];
	int styleNext = MaskActive(styler.StyleAt(startPos));
	int style = MaskActive(initStyle);
	const bool userDefinedFoldMarkers = !options.foldExplicitStart.empty() && !options.foldExplicitEnd.empty();
	for (Sci_PositionU i = startPos; i < endPos; i++) {
		const char ch = chNext;
		chNext = styler.SafeGetCharAt(i + 1);
		const int stylePrev = style;
		style = styleNext;
		styleNext = MaskActive(styler.StyleAt(i + 1));
		const bool atEOL = i == (lineStartNext-1);
		if ((style == SCE_C_COMMENTLINE) || (style == SCE_C_COMMENTLINEDOC))
			inLineComment = true;
		if (options.foldComment && options.foldCommentMultiline && IsStreamCommentStyle(style) && !inLineComment) {
			if (!IsStreamCommentStyle(stylePrev)) {
				levelNext++;
			} else if (!IsStreamCommentStyle(styleNext) && !atEOL) {
				// Comments don't end at end of line and the next character may be unstyled.
				levelNext--;
			}
		}
		if (options.foldComment && options.foldCommentExplicit && ((style == SCE_C_COMMENTLINE) || options.foldExplicitAnywhere)) {
			if (userDefinedFoldMarkers) {
				if (styler.Match(i, options.foldExplicitStart.c_str())) {
					levelNext++;
				} else if (styler.Match(i, options.foldExplicitEnd.c_str())) {
					levelNext--;
				}
			} else {
				if ((ch == '/') && (chNext == '/')) {
					const char chNext2 = styler.SafeGetCharAt(i + 2);
					if (chNext2 == '{') {
						levelNext++;
					} else if (chNext2 == '}') {
						levelNext--;
					}
				}
			}
		}
		if (options.foldPreprocessor && (style == SCE_C_PREPROCESSOR)) {
			if (ch == '#') {
				Sci_PositionU j = i + 1;
				while ((j < endPos) && IsASpaceOrTab(styler.SafeGetCharAt(j))) {
					j++;
				}
				if (styler.Match(j, "region") || styler.Match(j, "if")) {
					levelNext++;
				} else if (styler.Match(j, "end")) {
					levelNext--;
				}

				if (options.foldPreprocessorAtElse && (styler.Match(j, "else") || styler.Match(j, "elif"))) {
					levelMinCurrent--;
				}
			}
		}
		if (options.foldSyntaxBased && (style == SCE_C_OPERATOR)) {
			if (ch == '{' || ch == '[' || ch == '(') {
				// Measure the minimum before a '{' to allow
				// folding on "} else {"
				if (options.foldAtElse && levelMinCurrent > levelNext) {
					levelMinCurrent = levelNext;
				}
				levelNext++;
			} else if (ch == '}' || ch == ']' || ch == ')') {
				levelNext--;
			}
		}
		if (!IsASpace(ch))
			visibleChars++;
		if (atEOL || (i == endPos-1)) {
			int levelUse = levelCurrent;
			if ((options.foldSyntaxBased && options.foldAtElse) ||
				(options.foldPreprocessor && options.foldPreprocessorAtElse)
			) {
				levelUse = levelMinCurrent;
			}
			int lev = levelUse | levelNext << 16;
			if (visibleChars == 0 && options.foldCompact)
				lev |= SC_FOLDLEVELWHITEFLAG;
			if (levelUse < levelNext)
				lev |= SC_FOLDLEVELHEADERFLAG;
			if (lev != styler.LevelAt(lineCurrent)) {
				styler.SetLevel(lineCurrent, lev);
			}
			lineCurrent++;
			lineStartNext = styler.LineStart(lineCurrent+1);
			levelCurrent = levelNext;
			levelMinCurrent = levelCurrent;
			if (atEOL && (i == static_cast<Sci_PositionU>(styler.Length()-1))) {
				// There is an empty line at end of file so give it same level and empty
				styler.SetLevel(lineCurrent, (levelCurrent | levelCurrent << 16) | SC_FOLDLEVELWHITEFLAG);
			}
			visibleChars = 0;
			inLineComment = false;
		}
	}
}

void LexerNWScript::EvaluateTokens(std::vector<std::string> &tokens, const SymbolTable &preprocessorDefinitions) {

	// Remove whitespace tokens
	tokens.erase(std::remove_if(tokens.begin(), tokens.end(), OnlySpaceOrTab), tokens.end());

	// Evaluate defined statements to either 0 or 1
	for (size_t i=0; (i+1)<tokens.size();) {
		if (tokens[i] == "defined") {
			const char *val = "0";
			if (tokens[i+1] == "(") {
				if (((i + 2)<tokens.size()) && (tokens[i + 2] == ")")) {
					// defined()
					tokens.erase(tokens.begin() + i + 1, tokens.begin() + i + 3);
				} else if (((i+3)<tokens.size()) && (tokens[i+3] == ")")) {
					// defined(<identifier>)
					SymbolTable::const_iterator it = preprocessorDefinitions.find(tokens[i+2]);
					if (it != preprocessorDefinitions.end()) {
						val = "1";
					}
					tokens.erase(tokens.begin() + i + 1, tokens.begin() + i + 4);
				} else {
					// Spurious '(' so erase as more likely to result in false
					tokens.erase(tokens.begin() + i + 1, tokens.begin() + i + 2);
				}
			} else {
				// defined <identifier>
				SymbolTable::const_iterator it = preprocessorDefinitions.find(tokens[i+1]);
				if (it != preprocessorDefinitions.end()) {
					val = "1";
				}
				tokens.erase(tokens.begin() + i + 1, tokens.begin() + i + 2);
			}
			tokens[i] = val;
		} else {
			i++;
		}
	}

	// Evaluate identifiers
	constexpr size_t maxIterations = 100;
	size_t iterations = 0;	// Limit number of iterations in case there is a recursive macro.
	for (size_t i = 0; (i<tokens.size()) && (iterations < maxIterations);) {
		iterations++;
		if (setWordStart.Contains(tokens[i][0])) {
			SymbolTable::const_iterator it = preprocessorDefinitions.find(tokens[i]);
			if (it != preprocessorDefinitions.end()) {
				// Tokenize value
				std::vector<std::string> macroTokens = Tokenize(it->second.value);
				if (it->second.IsMacro()) {
					if ((i + 1 < tokens.size()) && (tokens.at(i + 1) == "(")) {
						// Create map of argument name to value
						std::vector<std::string> argumentNames = StringSplit(it->second.arguments, ',');
						std::map<std::string, std::string> arguments;
						size_t arg = 0;
						size_t tok = i+2;
						while ((tok < tokens.size()) && (arg < argumentNames.size()) && (tokens.at(tok) != ")")) {
							if (tokens.at(tok) != ",") {
								arguments[argumentNames.at(arg)] = tokens.at(tok);
								arg++;
							}
							tok++;
						}

						// Remove invocation
						tokens.erase(tokens.begin() + i, tokens.begin() + tok + 1);

						// Substitute values into macro
						macroTokens.erase(std::remove_if(macroTokens.begin(), macroTokens.end(), OnlySpaceOrTab), macroTokens.end());

						for (size_t iMacro = 0; iMacro < macroTokens.size();) {
							if (setWordStart.Contains(macroTokens[iMacro][0])) {
								std::map<std::string, std::string>::const_iterator itFind = arguments.find(macroTokens[iMacro]);
								if (itFind != arguments.end()) {
									// TODO: Possible that value will be expression so should insert tokenized form
									macroTokens[iMacro] = itFind->second;
								}
							}
							iMacro++;
						}

						// Insert results back into tokens
						tokens.insert(tokens.begin() + i, macroTokens.begin(), macroTokens.end());

					} else {
						i++;
					}
				} else {
					// Remove invocation
					tokens.erase(tokens.begin() + i);
					// Insert results back into tokens
					tokens.insert(tokens.begin() + i, macroTokens.begin(), macroTokens.end());
				}
			} else {
				// Identifier not found and value defaults to zero
				tokens[i] = "0";
			}
		} else {
			i++;
		}
	}

	// Find bracketed subexpressions and recurse on them
	BracketPair bracketPair = FindBracketPair(tokens);
	while (bracketPair.itBracket != tokens.end()) {
		std::vector<std::string> inBracket(bracketPair.itBracket + 1, bracketPair.itEndBracket);
		EvaluateTokens(inBracket, preprocessorDefinitions);

		// The insertion is done before the removal because there were failures with the opposite approach
		tokens.insert(bracketPair.itBracket, inBracket.begin(), inBracket.end());

		bracketPair = FindBracketPair(tokens);
		tokens.erase(bracketPair.itBracket, bracketPair.itEndBracket + 1);

		bracketPair = FindBracketPair(tokens);
	}

	// Evaluate logical negations
	for (size_t j=0; (j+1)<tokens.size();) {
		if (setNegationOp.Contains(tokens[j][0])) {
			int isTrue = atoi(tokens[j+1].c_str());
			if (tokens[j] == "!")
				isTrue = ~isTrue;			// TEST: Changed to bitwise
			std::vector<std::string>::iterator itInsert =
				tokens.erase(tokens.begin() + j, tokens.begin() + j + 2);
			tokens.insert(itInsert, isTrue ? "1" : "0");
		} else {
			j++;
		}
	}

	// Evaluate expressions in precedence order
	enum precedence { precMult, precAdd, precRelative
		, precLogical, /* end marker */ precLast };
	for (int prec = precMult; prec < precLast; prec++) {
		// Looking at 3 tokens at a time so end at 2 before end
		for (size_t k=0; (k+2)<tokens.size();) {
			const char chOp = tokens[k+1][0];
			if (
				((prec==precMult) && setMultOp.Contains(chOp)) ||
				((prec==precAdd) && setAddOp.Contains(chOp)) ||
				((prec==precRelative) && setRelOp.Contains(chOp)) ||
				((prec==precLogical) && setLogicalOp.Contains(chOp))
				) {
				const int valA = atoi(tokens[k].c_str());
				const int valB = atoi(tokens[k+2].c_str());
				int result = 0;
				if (tokens[k+1] == "+")
					result = valA + valB;
				else if (tokens[k+1] == "-")
					result = valA - valB;
				else if (tokens[k+1] == "*")
					result = valA * valB;
				else if (tokens[k+1] == "/")
					result = valA / (valB ? valB : 1);
				else if (tokens[k+1] == "%")
					result = valA % (valB ? valB : 1);
				else if (tokens[k+1] == "<")
					result = valA < valB;
				else if (tokens[k+1] == "<=")
					result = valA <= valB;
				else if (tokens[k+1] == ">")
					result = valA > valB;
				else if (tokens[k+1] == ">=")
					result = valA >= valB;
				else if (tokens[k+1] == "==")
					result = valA == valB;
				else if (tokens[k+1] == "!=")
					result = valA != valB;
				else if (tokens[k+1] == "||")
					result = valA | valB;                 // TEST: Changed to bitwise
				else if (tokens[k+1] == "&&")
					result = valA & valB;				  // TEST: Changed to bitwise
				std::vector<std::string>::iterator itInsert =
					tokens.erase(tokens.begin() + k, tokens.begin() + k + 3);
				tokens.insert(itInsert, std::to_string(result));
			} else {
				k++;
			}
		}
	}
}

std::vector<std::string> LexerNWScript::Tokenize(const std::string &expr) const {
	// Break into tokens
	std::vector<std::string> tokens;
	const char *cp = expr.c_str();
	while (*cp) {
		std::string word;
		if (setWord.Contains(*cp)) {
			// Identifiers and numbers
			while (setWord.Contains(*cp)) {
				word += *cp;
				cp++;
			}
		} else if (IsSpaceOrTab(*cp)) {
			while (IsSpaceOrTab(*cp)) {
				word += *cp;
				cp++;
			}
		} else if (setRelOp.Contains(*cp)) {
			word += *cp;
			cp++;
			if (setRelOp.Contains(*cp)) {
				word += *cp;
				cp++;
			}
		} else if (setLogicalOp.Contains(*cp)) {
			word += *cp;
			cp++;
			if (setLogicalOp.Contains(*cp)) {
				word += *cp;
				cp++;
			}
		} else {
			// Should handle strings, characters, and comments here
			word += *cp;
			cp++;
		}
		tokens.push_back(word);
	}
	return tokens;
}

bool LexerNWScript::EvaluateExpression(const std::string &expr, const SymbolTable &preprocessorDefinitions) {
	std::vector<std::string> tokens = Tokenize(expr);

	EvaluateTokens(tokens, preprocessorDefinitions);

	// "0" or "" -> false else true
	const bool isFalse = tokens.empty() ||
		((tokens.size() == 1) && ((tokens[0] == "") || tokens[0] == "0"));
	return !isFalse;
}

// Since we aren't registering a module Inside Notepad++ these functions bellow are not defined (implemented).
// If you were modding Notepad++ to include the language, then this should be set.
//LexerModule lmNWScript(SCLEX_NWSCRIPT, LexerNWScript::LexerFactoryNWScript, "nwscript", nwscriptWordLists);
//LexerModule lmNWScriptNoCase(SCLEX_NWSCRIPTNOCASE, LexerNWScript::LexerFactoryNWScriptInsensitive, "nwscriptnocase", nwscriptWordLists);


