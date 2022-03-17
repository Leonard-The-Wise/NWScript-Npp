//:://////////////////////////////////////////////////////////////////////////////////////////////////////
/** NWScript custom Lexer
 ** @file LexNWScript.h
 ** previous filename: LexCPP.h
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

#pragma once

 // When not defined, causes conflicts with std::max function used in NWScript Lexer
#ifndef NOMINMAX             
#undef max
#define NOMINMAX              
#endif

//#include <cstdlib>
//#include <cassert>
#include <map>
#include <string>
//#include <utility>
#include <vector>
 
#include "ILexer.h"
#include "SciLexer.h"
#include "Scintilla.h"
#include "Sci_Position.h"
 
#include "CharacterSet.h"
#include "LexAccessor.h"
#include "OptionSet.h"
#include "SparseState.h"
#include "StyleContext.h"
#include "SubStyles.h"
#include "WordList.h"
 
#include "LexerCatalogue.h"

// Extensions to SciLexer.h constants
#define SCE_C_ENGINETYPE 28
#define SCE_C_OBJECTTYPE 29
#define SCE_C_ENGINECONSTANT 30
#define SCE_C_USERCONSTANT 31
#define SCE_C_ENGINEFUNCTION 32
#define SCE_C_USERFUNCTION 36

// Identifiers for new lexers
#define SCLEX_NWSCRIPT (SCLEX_USER + 100)
#define SCLEX_NWSCRIPTNOCASE (SCLEX_USER + 101)

using namespace Scintilla;

// Giving an unamed namespace to protect functions and classes here from collision
namespace {

	constexpr bool IsSpaceEquiv(int state) noexcept {
		return (state <= SCE_C_COMMENTDOC) ||
			// including SCE_C_DEFAULT, SCE_C_COMMENT, SCE_C_COMMENTLINE
			(state == SCE_C_COMMENTLINEDOC) || (state == SCE_C_COMMENTDOCKEYWORD) ||
			(state == SCE_C_COMMENTDOCKEYWORDERROR);
	}

	// Preconditions: sc.currentPos points to a character after '+' or '-'.
	// The test for pos reaching 0 should be redundant,
	// and is in only for safety measures.
	// Limitation: this code will give the incorrect answer for code like
	// a = b+++/ptn/...
	// Putting a space between the '++' post-inc operator and the '+' binary op
	// fixes this, and is highly recommended for readability anyway.
	bool FollowsPostfixOperator(const StyleContext& sc, LexAccessor& styler) {
		Sci_Position pos = sc.currentPos;
		while (--pos > 0) {
			const char ch = styler[pos];
			if (ch == '+' || ch == '-') {
				return styler[pos - 1] == ch;
			}
		}
		return false;
	}

	bool followsReturnKeyword(const StyleContext& sc, LexAccessor& styler) {
		// Don't look at styles, so no need to flush.
		Sci_Position pos = sc.currentPos;
		const Sci_Position currentLine = styler.GetLine(pos);
		const Sci_Position lineStartPos = styler.LineStart(currentLine);
		while (--pos > lineStartPos) {
			const char ch = styler.SafeGetCharAt(pos);
			if (ch != ' ' && ch != '\t') {
				break;
			}
		}
		const char* retBack = "nruter";
		const char* s = retBack;
		while (*s
			&& pos >= lineStartPos
			&& styler.SafeGetCharAt(pos) == *s) {
			s++;
			pos--;
		}
		return !*s;
	}

	constexpr bool IsSpaceOrTab(int ch) noexcept {
		return ch == ' ' || ch == '\t';
	}

	bool OnlySpaceOrTab(const std::string& s) noexcept {
		for (const char ch : s) {
			if (!IsSpaceOrTab(ch))
				return false;
		}
		return true;
	}

	std::vector<std::string> StringSplit(const std::string& text, int separator) {
		std::vector<std::string> vs(text.empty() ? 0 : 1);
		for (const char ch : text) {
			if (ch == separator) {
				vs.emplace_back();
			}
			else {
				vs.back() += ch;
			}
		}
		return vs;
	}

	struct BracketPair {
		std::vector<std::string>::iterator itBracket;
		std::vector<std::string>::iterator itEndBracket;
	};

	BracketPair FindBracketPair(std::vector<std::string>& tokens) {
		BracketPair bp;
		std::vector<std::string>::iterator itTok = std::find(tokens.begin(), tokens.end(), "(");
		bp.itBracket = tokens.end();
		bp.itEndBracket = tokens.end();
		if (itTok != tokens.end()) {
			bp.itBracket = itTok;
			size_t nest = 0;
			while (itTok != tokens.end()) {
				if (*itTok == "(") {
					nest++;
				}
				else if (*itTok == ")") {
					nest--;
					if (nest == 0) {
						bp.itEndBracket = itTok;
						return bp;
					}
				}
				++itTok;
			}
		}
		bp.itBracket = tokens.end();
		return bp;
	}

	void highlightTaskMarker(StyleContext& sc, LexAccessor& styler,
		int activity, const WordList& markerList, bool caseSensitive) {
		if ((isoperator(sc.chPrev) || IsASpace(sc.chPrev)) && markerList.Length()) {
			constexpr Sci_PositionU lengthMarker = 50;
			char marker[lengthMarker + 1] = "";
			const Sci_PositionU currPos = sc.currentPos;
			Sci_PositionU i = 0;
			while (i < lengthMarker) {
				const char ch = styler.SafeGetCharAt(currPos + i);
				if (IsASpace(ch) || isoperator(ch)) {
					break;
				}
				if (caseSensitive)
					marker[i] = ch;
				else
					marker[i] = MakeLowerCase(ch);
				i++;
			}
			marker[i] = '\0';
			if (markerList.InList(marker)) {
				sc.SetState(SCE_C_TASKMARKER | activity);
			}
		}
	}

	class EscapeSequence {
		const CharacterSet setHexDigits = CharacterSet(CharacterSet::setDigits, "ABCDEFabcdef");
		const CharacterSet setOctDigits = CharacterSet(CharacterSet::setNone, "01234567");
		const CharacterSet setNoneNumeric;
		const CharacterSet* escapeSetValid = nullptr;
		int digitsLeft = 0;
	public:
		EscapeSequence() = default;
		void resetEscapeState(int nextChar) {
			digitsLeft = 0;
			escapeSetValid = &setNoneNumeric;
			if (nextChar == 'U') {
				digitsLeft = 9;
				escapeSetValid = &setHexDigits;
			}
			else if (nextChar == 'u') {
				digitsLeft = 5;
				escapeSetValid = &setHexDigits;
			}
			else if (nextChar == 'x') {
				digitsLeft = 5;
				escapeSetValid = &setHexDigits;
			}
			else if (setOctDigits.Contains(nextChar)) {
				digitsLeft = 3;
				escapeSetValid = &setOctDigits;
			}
		}
		bool atEscapeEnd(int currChar) const {
			return (digitsLeft <= 0) || !escapeSetValid->Contains(currChar);
		}
		void consumeDigit() noexcept {
			digitsLeft--;
		}
	};

	std::string GetRestOfLine(LexAccessor& styler, Sci_Position start, bool allowSpace) {
		std::string restOfLine;
		Sci_Position line = styler.GetLine(start);
		Sci_Position pos = start;
		Sci_Position endLine = styler.LineEnd(line);
		char ch = styler.SafeGetCharAt(start, '\n');
		while (pos < endLine) {
			if (ch == '\\' && ((pos + 1) == endLine)) {
				// Continuation line
				line++;
				pos = styler.LineStart(line);
				endLine = styler.LineEnd(line);
				ch = styler.SafeGetCharAt(pos, '\n');
			}
			else {
				const char chNext = styler.SafeGetCharAt(pos + 1, '\n');
				if (ch == '/' && (chNext == '/' || chNext == '*'))
					break;
				if (allowSpace || (ch != ' ')) {
					restOfLine += ch;
				}
				pos++;
				ch = chNext;
			}
		}
		return restOfLine;
	}

	constexpr bool IsStreamCommentStyle(int style) noexcept {
		return style == SCE_C_COMMENT ||
			style == SCE_C_COMMENTDOC ||
			style == SCE_C_COMMENTDOCKEYWORD ||
			style == SCE_C_COMMENTDOCKEYWORDERROR;
	}

	struct PPDefinition {
		Sci_Position line;
		std::string key;
		std::string value;
		bool isUndef;
		std::string arguments;
		PPDefinition(Sci_Position line_, const std::string& key_, const std::string& value_, bool isUndef_ = false, const std::string& arguments_ = "") :
			line(line_), key(key_), value(value_), isUndef(isUndef_), arguments(arguments_) {
		}
	};

	constexpr int inactiveFlag = 0x40;// 0x40;

	class LinePPState {
		// Track the state of preprocessor conditionals to allow showing active and inactive
		// code in different styles.
		// Only works up to 31 levels of conditional nesting.

		// state is a bit mask with 1 bit per level
		// bit is 1 for level if section inactive, so any bits set = inactive style
		int state = 0;
		// ifTaken is a bit mask with 1 bit per level
		// bit is 1 for level if some branch at this level has been taken
		int ifTaken = 0;
		// level is the nesting level of #if constructs
		int level = -1;
		static const int maximumNestingLevel = 31;
		bool ValidLevel() const noexcept {
			return level >= 0 && level < maximumNestingLevel;
		}
		int maskLevel() const noexcept {
			if (level >= 0) {
				return 1 << level;
			}
			else {
				return 1;
			}
		}
	public:
		LinePPState() noexcept {
		}
		bool IsActive() const noexcept {
			return state == 0;
		}
		bool IsInactive() const noexcept {
			return state != 0;
		}
		int ActiveState() const noexcept {
			return state ? inactiveFlag : 0;
		}
		bool CurrentIfTaken() const noexcept {
			return (ifTaken & maskLevel()) != 0;
		}
		void StartSection(bool on) noexcept {
			level++;
			if (ValidLevel()) {
				if (on) {
					state &= ~maskLevel();
					ifTaken |= maskLevel();
				}
				else {
					state |= maskLevel();
					ifTaken &= ~maskLevel();
				}
			}
		}
		void EndSection() noexcept {
			if (ValidLevel()) {
				state &= ~maskLevel();
				ifTaken &= ~maskLevel();
			}
			level--;
		}
		void InvertCurrentLevel() noexcept {
			if (ValidLevel()) {
				state ^= maskLevel();
				ifTaken |= maskLevel();
			}
		}
	};

	// Hold the preprocessor state for each line seen.
	// Currently one entry per line but could become sparse with just one entry per preprocessor line.
	class PPStates {
		std::vector<LinePPState> vlls;
	public:
		LinePPState ForLine(Sci_Position line) const noexcept {
			if ((line > 0) && (vlls.size() > static_cast<size_t>(line))) {
				return vlls[line];
			}
			else {
				return LinePPState();
			}
		}
		void Add(Sci_Position line, LinePPState lls) {
			vlls.resize(line + 1);
			vlls[line] = lls;
		}
	};

	// An individual named option for use in an OptionSet

	// Options used for LexerNWScript
	struct OptionsNWScript {
		bool stylingWithinPreprocessor;
		bool identifiersAllowDollars;
		bool trackPreprocessor;
		bool updatePreprocessor;
		bool verbatimStringsAllowEscapes;
		bool triplequotedStrings;
		bool hashquotedStrings;
		bool backQuotedStrings;
		bool escapeSequence;
		bool fold;
		bool foldSyntaxBased;
		bool foldComment;
		bool foldCommentMultiline;
		bool foldCommentExplicit;
		std::string foldExplicitStart;
		std::string foldExplicitEnd;
		bool foldExplicitAnywhere;
		bool foldPreprocessor;
		bool foldPreprocessorAtElse;
		bool foldCompact;
		bool foldAtElse;
		OptionsNWScript() {
			stylingWithinPreprocessor = false;
			identifiersAllowDollars = true;
			trackPreprocessor = true;
			updatePreprocessor = true;
			verbatimStringsAllowEscapes = false;
			triplequotedStrings = false;
			hashquotedStrings = false;
			backQuotedStrings = false;
			escapeSequence = false;
			fold = true;
			foldSyntaxBased = true;
			foldComment = true;
			foldCommentMultiline = true;
			foldCommentExplicit = true;
			foldExplicitStart = "";
			foldExplicitEnd = "";
			foldExplicitAnywhere = false;
			foldPreprocessor = false;
			foldPreprocessorAtElse = false;
			foldCompact = false;
			foldAtElse = false;
		}
	};

	const char* const nwscriptWordLists[] = {
				"Primary keywords and instruction set",
				"Primary variable types",
				"Documentation comment keywords",
				"Engine reserved types",
				"Engine reserved object class",
				"Engine defined constants",
				"User defined constants",
				"Engine internal functions",
				"User defined functions",
				nullptr,
	};

	struct OptionSetNWScript : public OptionSet<OptionsNWScript> {
		OptionSetNWScript() {
			DefineProperty("styling.within.preprocessor", &OptionsNWScript::stylingWithinPreprocessor,
				"Determines whether all preprocessor code is styled in the "
				"preprocessor style (0, the default) or only from the initial # to the end "
				"of the command word(1).");

			DefineProperty("lexer.nwscript.allow.dollars", &OptionsNWScript::identifiersAllowDollars,
				"Set to 0 to disallow the '$' character in identifiers with the nwscript lexer.");

			DefineProperty("lexer.nwscript.track.preprocessor", &OptionsNWScript::trackPreprocessor,
				"Set to 1 to interpret #if/#else/#endif to grey out code that is not active.");

			DefineProperty("lexer.nwscript.update.preprocessor", &OptionsNWScript::updatePreprocessor,
				"Set to 1 to update preprocessor definitions when #define found.");

			DefineProperty("lexer.nwscript.verbatim.strings.allow.escapes", &OptionsNWScript::verbatimStringsAllowEscapes,
				"Set to 1 to allow verbatim strings to contain escape sequences.");

			DefineProperty("lexer.nwscript.triplequoted.strings", &OptionsNWScript::triplequotedStrings,
				"Set to 1 to enable highlighting of triple-quoted strings.");

			DefineProperty("lexer.nwscript.hashquoted.strings", &OptionsNWScript::hashquotedStrings,
				"Set to 1 to enable highlighting of hash-quoted strings.");

			DefineProperty("lexer.nwscript.backquoted.strings", &OptionsNWScript::backQuotedStrings,
				"Set to 1 to enable highlighting of back-quoted raw strings .");

			DefineProperty("lexer.nwscript.escape.sequence", &OptionsNWScript::escapeSequence,
				"Set to 1 to enable highlighting of escape sequences in strings");

			DefineProperty("fold", &OptionsNWScript::fold);

			DefineProperty("fold.nwscript.syntax.based", &OptionsNWScript::foldSyntaxBased,
				"Set this property to 0 to disable syntax based folding.");

			DefineProperty("fold.comment", &OptionsNWScript::foldComment,
				"This option enables folding multi-line comments and explicit fold points when using the C++ lexer. "
				"Explicit fold points allows adding extra folding by placing a //{ comment at the start and a //} "
				"at the end of a section that should fold.");

			DefineProperty("fold.nwscript.comment.multiline", &OptionsNWScript::foldCommentMultiline,
				"Set this property to 0 to disable folding multi-line comments when fold.comment=1.");

			DefineProperty("fold.nwscript.comment.explicit", &OptionsNWScript::foldCommentExplicit,
				"Set this property to 0 to disable folding explicit fold points when fold.comment=1.");

			DefineProperty("fold.nwscript.explicit.start", &OptionsNWScript::foldExplicitStart,
				"The string to use for explicit fold start points, replacing the standard //{.");

			DefineProperty("fold.nwscript.explicit.end", &OptionsNWScript::foldExplicitEnd,
				"The string to use for explicit fold end points, replacing the standard //}.");

			DefineProperty("fold.nwscript.explicit.anywhere", &OptionsNWScript::foldExplicitAnywhere,
				"Set this property to 1 to enable explicit fold points anywhere, not just in line comments.");

			DefineProperty("fold.nwscript.preprocessor.at.else", &OptionsNWScript::foldPreprocessorAtElse,
				"This option enables folding on a preprocessor #else or #endif line of an #if statement.");

			DefineProperty("fold.preprocessor", &OptionsNWScript::foldPreprocessor,
				"This option enables folding preprocessor directives when using the C++ lexer. "
				"Includes C#'s explicit #region and #endregion folding directives.");

			DefineProperty("fold.compact", &OptionsNWScript::foldCompact);

			DefineProperty("fold.at.else", &OptionsNWScript::foldAtElse,
				"This option enables C++ folding on a \"} else {\" line of an if statement.");

			DefineWordListSets(nwscriptWordLists);
		}
	};

	const char styleSubable[] = { SCE_C_IDENTIFIER, SCE_C_COMMENTDOCKEYWORD, 0 };

	LexicalClass lexicalClasses[] = {
		// Lexer Cpp SCLEX_CPP SCE_C_: maintainad 'as-is', but added extensions
		0, "SCE_C_DEFAULT", "default", "White space",
		1, "SCE_C_COMMENT", "comment", "Comment: /* */.",
		2, "SCE_C_COMMENTLINE", "comment line", "Line Comment: //.",
		3, "SCE_C_COMMENTDOC", "comment documentation", "Doc comment: block comments beginning with /** or /*!",
		4, "SCE_C_NUMBER", "literal numeric", "Number",
		5, "SCE_C_WORD", "keyword", "Instruction Set",
		6, "SCE_C_STRING", "literal string", "Double quoted string",
		7, "SCE_C_CHARACTER", "literal string character", "Single quoted string",
		8, "SCE_C_UUID", "literal uuid", "UUIDs (only in IDL)",
		9, "SCE_C_PREPROCESSOR", "preprocessor", "Preprocessor",
		10, "SCE_C_OPERATOR", "operator", "Operators",
		11, "SCE_C_IDENTIFIER", "identifier", "Identifiers",
		12, "SCE_C_STRINGEOL", "error literal string", "End of line where string is not closed",
		13, "SCE_C_VERBATIM", "literal string multiline raw", "Verbatim strings for C#",
		14, "SCE_C_REGEX", "literal regex", "Regular expressions for JavaScript",
		15, "SCE_C_COMMENTLINEDOC", "comment documentation line", "Doc Comment Line: line comments beginning with /// or //!.",
		16, "SCE_C_WORD2", "identifier", "Instruction Set 2 - Unused",
		17, "SCE_C_COMMENTDOCKEYWORD", "comment documentation keyword", "Comment keyword",
		18, "SCE_C_COMMENTDOCKEYWORDERROR", "error comment documentation keyword", "Comment keyword error",
		19, "SCE_C_GLOBALCLASS", "identifier", "Global class",
		20, "SCE_C_STRINGRAW", "literal string multiline raw", "Raw strings for C++0x",
		21, "SCE_C_TRIPLEVERBATIM", "literal string multiline raw", "Triple-quoted strings for Vala",
		22, "SCE_C_HASHQUOTEDSTRING", "literal string", "Hash-quoted strings for Pike",
		23, "SCE_C_PREPROCESSORCOMMENT", "comment preprocessor", "Preprocessor stream comment",
		24, "SCE_C_PREPROCESSORCOMMENTDOC", "comment preprocessor documentation", "Preprocessor stream doc comment",
		25, "SCE_C_USERLITERAL", "literal", "User defined literals",
		26, "SCE_C_TASKMARKER", "comment taskmarker", "Task Marker",
		27, "SCE_C_ESCAPESEQUENCE", "escape", "Escape sequence",
		28, "SCE_C_ENGINETYPE", "engine type", "Engine type",
		29, "SCE_C_OBJECTTYPE", "object type", "Object type",
		30, "SCE_C_ENGINECONSTANT", "engine constant", "Engine Constant",    // begin of extensions
		31, "SCE_C_USERCONSTANT", "user constant", "User Defined Constant",
		32, "SCE_C_ENGINEFUNCTION", "engine function", "Engine Function",
		36, "SCE_C_USERFUNCTION", "user function", "User Defined Function",  // Jump to 36. 33 would get overlaped by global style
	};

	const int sizeLexicalClasses = static_cast<int>(std::size(lexicalClasses));

}

class LexerNWScript : public ILexer5 {
	bool caseSensitive;
	CharacterSet setWord;
	CharacterSet setNegationOp;
	CharacterSet setAddOp;
	CharacterSet setMultOp;
	CharacterSet setRelOp;
	CharacterSet setLogicalOp;
	CharacterSet setWordStart;
	PPStates vlls;
	std::vector<PPDefinition> ppDefineHistory;
	WordList keywordsInstructions;		// keywords  - instruction set
	WordList keywordsInstr2;			// keywords2 - unused by NWScript
	WordList keywordsCommonTypes;		// keywords3 - common types (const float int string struct vector void)
	WordList keywordsEngineTypes;		// keywords4 - engine defined types (effect event location talent itemproperty sqlquery cassowary json)
	WordList keywordsObjectTypes;		// keywords5 - object exclusive word
	WordList keywordsEngineConstants;	// keywords6 - engine constants
	WordList keywordsUserConstants;		// keywords7 - user defined constants
	WordList keywordsEngineFunctions;	// keywords8 - engine functions
	WordList keywordsUserFunctions;		// keywords8 - user defined functions

	WordList markerList;
	WordList ppDefinitions;

	struct SymbolValue {
		std::string value;
		std::string arguments;
		SymbolValue() noexcept = default;
		SymbolValue(const std::string& value_, const std::string& arguments_) : value(value_), arguments(arguments_) {
		}
		SymbolValue& operator = (const std::string& value_) {
			value = value_;
			arguments.clear();
			return *this;
		}
		bool IsMacro() const noexcept {
			return !arguments.empty();
		}
	};
	typedef std::map<std::string, SymbolValue> SymbolTable;
	SymbolTable preprocessorDefinitionsStart;
	OptionsNWScript options;
	OptionSetNWScript osNWScript;
	EscapeSequence escapeSeq;
	SparseState<std::string> rawStringTerminators;
	enum { ssIdentifier, ssDocKeyword };
	SubStyles subStyles;
	std::string returnBuffer;
public:
	explicit LexerNWScript(bool caseSensitive_) :
		caseSensitive(caseSensitive_),
		setWord(CharacterSet::setAlphaNum, "._", 0x80, true),
		setNegationOp(CharacterSet::setNone, "!"),
		setAddOp(CharacterSet::setNone, "+-"),
		setMultOp(CharacterSet::setNone, "/*%"),
		setRelOp(CharacterSet::setNone, "=!<>"),
		setLogicalOp(CharacterSet::setNone, "|&"),
		subStyles(styleSubable, 0x80, 0x40, inactiveFlag) {
	}
	// Deleted so LexerNWScript objects can not be copied.
	LexerNWScript(const LexerNWScript&) = delete;
	LexerNWScript(LexerNWScript&&) = delete;
	void operator=(const LexerNWScript&) = delete;
	void operator=(LexerNWScript&&) = delete;
	virtual ~LexerNWScript() {
	}
	void SCI_METHOD Release() noexcept override {
		delete this;
	}
	int SCI_METHOD Version() const noexcept override {
		return lvRelease5;
	}
	const char* SCI_METHOD PropertyNames() override {
		return osNWScript.PropertyNames();
	}
	int SCI_METHOD PropertyType(const char* name) override {
		return osNWScript.PropertyType(name);
	}
	const char* SCI_METHOD DescribeProperty(const char* name) override {
		return osNWScript.DescribeProperty(name);
	}
	Sci_Position SCI_METHOD PropertySet(const char* key, const char* val) override;
	const char* SCI_METHOD DescribeWordListSets() override {
		return osNWScript.DescribeWordListSets();
	}
	Sci_Position SCI_METHOD WordListSet(int n, const char* wl) override;
	void SCI_METHOD Lex(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;
	void SCI_METHOD Fold(Sci_PositionU startPos, Sci_Position length, int initStyle, IDocument* pAccess) override;

	void* SCI_METHOD PrivateCall(int, void*) noexcept override {
		return nullptr;
	}

	int SCI_METHOD LineEndTypesSupported() noexcept override {
		return SC_LINE_END_TYPE_UNICODE;
	}

	int SCI_METHOD AllocateSubStyles(int styleBase, int numberStyles) override {
		return subStyles.Allocate(styleBase, numberStyles);
	}
	int SCI_METHOD SubStylesStart(int styleBase) override {
		return subStyles.Start(styleBase);
	}
	int SCI_METHOD SubStylesLength(int styleBase) override {
		return subStyles.Length(styleBase);
	}
	int SCI_METHOD StyleFromSubStyle(int subStyle) override {
		const int styleBase = subStyles.BaseStyle(MaskActive(subStyle));
		const int inactive = subStyle & inactiveFlag;
		return styleBase | inactive;
	}
	int SCI_METHOD PrimaryStyleFromStyle(int style) noexcept override {
		return MaskActive(style);
	}
	void SCI_METHOD FreeSubStyles() override {
		subStyles.Free();
	}
	void SCI_METHOD SetIdentifiers(int style, const char* identifiers) override {
		subStyles.SetIdentifiers(style, identifiers);
	}
	int SCI_METHOD DistanceToSecondaryStyles() noexcept override {
		return inactiveFlag;
	}
	const char* SCI_METHOD GetSubStyleBases() noexcept override {
		return styleSubable;
	}
	int SCI_METHOD NamedStyles() override {
		return std::max(subStyles.LastAllocated() + 1,
			sizeLexicalClasses) +
			inactiveFlag;
	}
	const char* SCI_METHOD NameOfStyle(int style) override {
		if (style >= NamedStyles())
			return "";
		if (style < sizeLexicalClasses)
			return lexicalClasses[style].name;
		return "";
	}
	const char* SCI_METHOD TagsOfStyle(int style) override {
		if (style >= NamedStyles())
			return "Excess";
		returnBuffer.clear();
		const int firstSubStyle = subStyles.FirstAllocated();
		if (firstSubStyle >= 0) {
			const int lastSubStyle = subStyles.LastAllocated();
			if (((style >= firstSubStyle) && (style <= (lastSubStyle))) ||
				((style >= firstSubStyle + inactiveFlag) && (style <= (lastSubStyle + inactiveFlag)))) {
				int styleActive = style;
				if (style > lastSubStyle) {
					returnBuffer = "inactive ";
					styleActive -= inactiveFlag;
				}
				const int styleMain = StyleFromSubStyle(styleActive);
				returnBuffer += lexicalClasses[styleMain].tags;
				return returnBuffer.c_str();
			}
		}
		if (style < sizeLexicalClasses)
			return lexicalClasses[style].tags;
		if (style >= inactiveFlag) {
			returnBuffer = "inactive ";
			const int styleActive = style - inactiveFlag;
			if (styleActive < sizeLexicalClasses)
				returnBuffer += lexicalClasses[styleActive].tags;
			else
				returnBuffer = "";
			return returnBuffer.c_str();
		}
		return "";
	}
	const char* SCI_METHOD DescriptionOfStyle(int style) override {
		if (style >= NamedStyles())
			return "";
		if (style < sizeLexicalClasses)
			return lexicalClasses[style].description;
		return "";
	}

	// ILexer5 methods
	const char* SCI_METHOD GetName() override {
		return caseSensitive ? "NWScript" : "NWScriptNoCase";
	}
	int SCI_METHOD GetIdentifier() override {
		return caseSensitive ? SCLEX_NWSCRIPT : SCLEX_NWSCRIPTNOCASE;
	}
	const char* SCI_METHOD PropertyGet(const char* key) override;

	static ILexer5* LexerFactoryNWScript() {
		return new LexerNWScript(true);
	}
	static ILexer5* LexerFactoryNWScriptInsensitive() {
		return new LexerNWScript(false);
	}
	constexpr static int MaskActive(int style) noexcept {
		return style & ~inactiveFlag;
	}
	void EvaluateTokens(std::vector<std::string>& tokens, const SymbolTable& preprocessorDefinitions);
	std::vector<std::string> Tokenize(const std::string& expr) const;
	bool EvaluateExpression(const std::string& expr, const SymbolTable& preprocessorDefinitions);
};

