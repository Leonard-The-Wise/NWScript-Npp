/** @file NWScriptParser.cpp
 * Parser for a NWScript.nss file
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"

//#include <assert.h>
//#include <memory>
//#include <string>
//#include <sstream>
//#include <algorithm>
//#include <utility>
//#include <tchar.h>
//#include <sys/stat.h>
//#include <fstream>
//#include <cctype>
//#include <iterator>
//#include <locale>
//#include <ShlObj.h>

#include "jpcre2.hpp"
#include "Utf8_16.h"
#include "NWScriptParser.h"


//:://///////////////////////////////////////////////////////////
// Comments on RegEx by the author...
// I've built all regular expressions used here within this site:
// https://regex101.com/
// 
// I'm not affiliated with them in anyway. In fact, I felt
// like donating there for the fantastic debbuggin features
// and testing they provided. Without their help, this task would
// be exponentialy harder and time consuming.
//:://///////////////////////////////////////////////////////////

// Robust regex definitions are slower, but can handle object nesting, 
// inline comments and validates functions parameters. But since PCRE2
// engine was SO fast in parsing it, the performance degratation was all but
// neglectable, so now we'll use ROBUST definitions always. We also optimized
// the functions, stripping any comments from the file before parsing, so now
// we don't need to check for inline comments inside declarations and function scoping,
// so the performance bump was enormous, hence mitigating ALL overhead we had before.

#define ROBUSTREGEXDEFINITIONS
#ifdef ROBUSTREGEXDEFINITIONS
const std::string BASEREGEX = R"((?(DEFINE)(?<word>[\w\d.\-]++)(?<string>"(?>\\.|[^"\\]*+)*+")(?<token>\g<word>|\g<string>)(?<tokenVector>\[\s*+(?>\g<validValue>(?=\])|\g<validValue>,(?=\g<validValue>))*+\])(?<object>\{\s*+(?>\g<validValue>(?=\})|\g<validValue>,(?=\g<validValue>))*+\})(?<validValue>\s*+(?>\g<token>|\g<tokenVector>|\g<object>)\s*+)(?'param'\s*+(?>const)?\s*+(?#paramType)\w+\s*+(?#paramName)\w+\s*+(?>=\s*+(?#paramDefaultValue)\g<validValue>)?\s*+)(?<fnContents>{(?:[^{"}]*+|\g<string>|\g<fnContents>)*})))";
const std::string COMMENTSREGEX = R"((?(DEFINE)(?'commentLine'\/\/.*+)(?'comment'\/\*(?>\*\/|(?>(?>.|\n)(?!\*\/))*+)(?>(?>.|\n)(?=\*\/)\*\/)?)(?'cnotnull'(?>\g<commentLine>|\g<comment>)++)(?'c'\g<cnotnull>?))\g<cnotnull>)";
const std::string ENGINESTRUCTREGEX = R"(^\s*+\K(?>#define)\s++(?>ENGINE_STRUCTURE_\d++)\s++(?<name>\w++))";
const std::string FUNCTIONDECLARATIONREGEX = BASEREGEX + R"(^\s*+\K(?<type>\w+)\s*+(?<name>\w+)\s*+\((?<parametersString>(?>\g<param>(?=\))|\g<param>,(?=\g<param>))*+)\)\s*+;)";
const std::string FUNCTIONSDEFINITIONREGEX = BASEREGEX + R"(^\s*+\K(?>\w+)\s*+(?>\w+)\s*+\((?>(?>\g<param>,(?=\g<param>)|\g<param>(?=\))))*+\)\s*+\g<fnContents>)";
const std::string FUNTIONPARAMETERREGEX = BASEREGEX + R"(\s*+(?>const)?\s*+(?'type'\w+)\s*+(?'name'\w+)\s*+(?>=\s*+(?'defaultValue'\g<validValue>))?\s*+,?)";
const std::string CONSTANTREGEX = BASEREGEX + R"(^\s*+(?>const)?\s*+\K(?<type>\w+)\s*+(?<name>\w+)\s*+=\s*+(?<value>\g<validValue>)\s*+;)";
const std::string KEYWORDREGEX = R"(#?\w+)";
#else
const std::string BASEREGEX(R"((?(DEFINE)(?<token>(?>("(?:\\.|[^"\\])*"|[\w\d.\-]+)))(?<tokenVector>(\[(?>\s*+(?>\g<token>|\g{5})\s*+,?)*+\s*+\]))(?<object>(\{(?>\s*+(?>\g<token>|\g<tokenVector>)\s*+,?)*+\s*+\}))))");
const std::string ENGINESTRUCTREGEX(R"(^\s*+\K(?>#define)\s++(?>ENGINE_STRUCTURE_\d++)\s++(?<name>\w++))");
const std::string FUNCTIONDECLARATIONREGEX(R"(^\s*+(?<type>(?!(?>return|if|else))\w++)\s++(?<name>\w+)\s*+\(\s*+(?<parametersString>(?>[\w\s=\-\.\[\]\,\{\}]|(?>"(?>\\.|[^"\\])*+")*+)*+)\)\s*+;)");
const std::string FUNTIONPARAMETERREGEX = BASEREGEX + R"((?>const){0,1}\s*+(?<type>\w++)\s++(?<name>\w++)\s*+(?>=\s*+(?<defaultValue>(?>(?>\g<token>|\g<tokenVector>|\g<object>))))?,?)";
const std::string CONSTANTREGEX = BASEREGEX + R"(^\s*+(?>const)?\s*+(?<type>\w++)\s++(?<name>\w++)\s*+=?\s*+(?<value>\g<token>|\g<tokenVector>|\g<object>)\s*+;)";
#endif

// We create and compile regexes only once during code initialization
static const jpcre2::select<char>::Regex commentsRegEx(COMMENTSREGEX, PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<char>::Regex engineStructRegEx(ENGINESTRUCTREGEX, PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<char>::Regex functionsDeclarationRegEx(FUNCTIONDECLARATIONREGEX, PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<char>::Regex functionsDefinitionRegEx(FUNCTIONSDEFINITIONREGEX, PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<char>::Regex functionsParamRegEx(FUNTIONPARAMETERREGEX, PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<char>::Regex constantsRegEx(CONSTANTREGEX, PCRE2_MULTILINE, jpcre2::JIT_COMPILE);

static const jpcre2::select<wchar_t>::Regex commentsRegExW(str2wstr(COMMENTSREGEX), PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<wchar_t>::Regex engineStructRegExW(str2wstr(ENGINESTRUCTREGEX), PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<wchar_t>::Regex functionsDeclarationRegExW(str2wstr(FUNCTIONDECLARATIONREGEX), PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<wchar_t>::Regex functionsDefinitionRegExW(str2wstr(FUNCTIONSDEFINITIONREGEX), PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<wchar_t>::Regex functionsParamRegExW(str2wstr(FUNTIONPARAMETERREGEX), PCRE2_MULTILINE, jpcre2::JIT_COMPILE);
static const jpcre2::select<wchar_t>::Regex constantsRegExW(str2wstr(CONSTANTREGEX), PCRE2_MULTILINE, jpcre2::JIT_COMPILE);

static const jpcre2::select<wchar_t>::Regex keywordImportW(str2wstr(KEYWORDREGEX), PCRE2_MULTILINE, jpcre2::JIT_COMPILE);

constexpr const int blockSize = 128 * 1024 + 4;

using namespace NWScriptPlugin;

void NWScriptParser::ScriptParseResults::Sort() {
	std::sort(Members.begin(), Members.end(),
		[](ScriptMember a, ScriptMember b) {  return a.sName < b.sName;});
}

bool NWScriptParser::ParseFile(const generic_string& sFileName, ScriptParseResults& outParseResults)
{
	// First resolve possible file link
	const rsize_t longFileNameBufferSize = MAX_PATH; 
	if (sFileName.size() >= longFileNameBufferSize - 1) 
		return false;

	generic_string targetFileName = sFileName;
	resolveLinkFile(targetFileName);
	TCHAR longFileName[longFileNameBufferSize];

	const DWORD getFullPathNameResult = ::GetFullPathName(targetFileName.c_str(), longFileNameBufferSize, longFileName, NULL);
	if (getFullPathNameResult == 0)
	{
		return false;
	}
	if (getFullPathNameResult > longFileNameBufferSize)
	{
		return false;
	}
	assert(_tcslen(longFileName) == getFullPathNameResult);

	if (_tcschr(longFileName, '~'))
	{
		// ignore the returned value of function due to win64 redirection system
		::GetLongPathName(longFileName, longFileName, longFileNameBufferSize);
	}

	// Reconvert back filename to stop working with TCHAR pointers
	targetFileName = longFileName;
	
	// Read the raw file contents
	std::string sFileContents;
	bool success = fileToBuffer(targetFileName, sFileContents);

	// Convert unicode files
	Utf8_16_Read encoder;
	int encoding = encoder.determineEncoding((unsigned char*)sFileContents.c_str(), (blockSize > sFileContents.size()) ? sFileContents.size() : blockSize);
	if (encoding == uni16BE || encoding == uni16LE || encoding == uni16BE_NoBOM || encoding == uni16LE_NoBOM)
	{
		std::ignore = encoder.convert(sFileContents.data(), sFileContents.size());
		sFileContents.assign(encoder.getNewBuf(), encoder.getNewSize());
	}

	// Create file structure
	CreateNWScriptStructure(sFileContents, outParseResults);

	return true;

}

bool NWScriptParser::ParseBatch(const std::vector<generic_string>& sFilePaths, ScriptParseResults& outParseResults)
{
	for (const generic_string& path : sFilePaths)
	{
		if (!ParseFile(path, outParseResults))
			return false;
	}

	return true;
}

void NWScriptParser::CreateNWScriptStructure(const std::string& sFileContents, ScriptParseResults& outParseResults)
{
	typedef jpcre2::select<char> pcre2;

	// Reserve a good amount of space since we're not doing dynamic reallocations. 1 member per line is more than enough
	// Try standard EOL mode (\n). Then alternative (\r). Minimum to reserve is assumed to 1 member since at least 1 line 
	// will be processed by the RegEx.
	size_t lineCount = std::count(sFileContents.begin(), sFileContents.end(), '\n');
	if (lineCount == 0)
		lineCount = std::count(sFileContents.begin(), sFileContents.end(), '\r');

	outParseResults.Members.reserve(outParseResults.Members.size() + 1 + lineCount);

	// First we strip all comments - even malformed ones, so we can use faster regexes for the rest
	std::string cleanFile = pcre2::Regex(commentsRegEx).replace(sFileContents, "", "gm");
	// And then we strip function definitions - so we don't catch any scoped variable.
	cleanFile = pcre2::Regex(functionsDefinitionRegEx).replace(cleanFile, "", "gm");

	// Then we create the capture groups and first step: engine structs.
	pcre2::VecNas captureGroup;
	pcre2::RegexMatch regexMatch(&engineStructRegEx);
	regexMatch.setSubject(cleanFile);
	regexMatch.addModifier("gm");
	regexMatch.setNamedSubstringVector(&captureGroup);
	size_t count = regexMatch.match();

	// Iterate through results. Since we're expecting generic_strings here
	// we also do character conversion.
	outParseResults.EngineStructuresCount += count;
	for (size_t i = 0; i < count; i++)
	{
		NWScriptParser::ScriptMember n;
		n.mID = MemberID::EngineStruct; n.sName = str2wstr(captureGroup[i]["name"]);
		outParseResults.Members.emplace_back(n);
	}

	regexMatch.setRegexObject(&functionsDeclarationRegEx);
	count = regexMatch.match();

	outParseResults.FunctionsCount += count;

	pcre2::RegexMatch regexMatchSubString(&functionsParamRegEx);
	regexMatchSubString.addModifier("gm");
	pcre2::VecNas captureSubGroup;
	regexMatchSubString.setNamedSubstringVector(&captureSubGroup);
	for (size_t i = 0; i < count; i++)
	{
		std::vector<ScriptParamMember> params;

		regexMatchSubString.setSubject(captureGroup[i]["parametersString"]);
		size_t nParams = regexMatchSubString.match();
		for (size_t j = 0; j < nParams; j++)
		{
			ScriptParamMember s;
			s.sType = str2wstr(captureSubGroup[j]["type"]); s.sName = str2wstr(captureSubGroup[j]["name"]);
			s.sDefaultValue = str2wstr(captureSubGroup[j]["defaultValue"]);
			params.push_back(s);
		}

		NWScriptParser::ScriptMember Me;
		Me.mID = MemberID::Function; Me.sType = str2wstr(captureGroup[i]["type"]);
		Me.sName = str2wstr(captureGroup[i]["name"]); Me.params = params;
		outParseResults.Members.emplace_back(Me);
	}

	regexMatch.setRegexObject(&constantsRegEx);
	count = regexMatch.match();

	outParseResults.ConstantsCount += count;
	for (size_t i = 0; i < count; i++)
	{
		NWScriptParser::ScriptMember n;
		n.mID = MemberID::Constant; n.sType = str2wstr(captureGroup[i]["type"]); n.sName = str2wstr(captureGroup[i]["name"]);
		n.sValue = str2wstr(captureGroup[i]["value"]);;
		outParseResults.Members.emplace_back(n);
	}

	outParseResults.Members.shrink_to_fit();
}

void NWScriptParser::ScriptParseResults::AddSpacedStringAsMember(const generic_string& sKWArray, MemberID memberID)
{
	typedef jpcre2::select<wchar_t> pcre2;

	pcre2::RegexMatch regexMatch(&keywordImportW);
	pcre2::VecNum matches;
	regexMatch.setSubject(sKWArray);
	regexMatch.addModifier("gm");
	regexMatch.setNumberedSubstringVector(&matches);

	size_t count = regexMatch.match();

	Members.reserve(Members.size() + count);
	for (size_t i = 0; i < count; i++)
	{
		ScriptMember m;
		m.mID = memberID; m.sName = matches[i][0];

		// Don't add duplicates
		if (std::find(Members.begin(), Members.end(), m) != Members.end())
			Members.emplace_back(m);
	}
}

bool NWScriptParser::ScriptParseResults::SerializeToFile(generic_string filePath)
{
	return true;
}

bool NWScriptParser::ScriptParseResults::SerializeFromFile(generic_string filePath)
{
	return true;
}
