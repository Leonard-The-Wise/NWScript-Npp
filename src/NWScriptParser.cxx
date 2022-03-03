/** @file NWScriptParser.cxx
 * Parser for a NWScript.nss file
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <memory>
#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include <tchar.h>
#include <sys/stat.h>
#include <fstream>
#include <cctype>
#include <iterator>
#include <locale>
#include <codecvt>

// #define USEADVANCEDENCODINGDETECTION
#ifdef USEADVANCEDENCODINGDETECTION
#include "uchardet.h"
#endif

// Replacing std::regex with boost because of crappy performance (10x increase!!!!).
//#include <regex>
#include "boost/regex.hpp"
#include "Utf8_16.h"

#include "NWScriptParser.h"

#pragma warning(disable : 4996 6387)

using namespace NWScriptPlugin;

using wregex = boost::basic_regex<wchar_t>;
using wregexmatch = boost::match_results<std::wstring::const_iterator>;
using wsregex_iterator = boost::regex_iterator<std::wstring::const_iterator>;
using wsregex_token_iterator = boost::regex_token_iterator<std::wstring::const_iterator>;
using wmatch = boost::match_results<std::wstring>;

static const wregex sEngineStructRegExW(L"^\\s*?((?:#define))\\s*((?:ENGINE_STRUCTURE))_\\d*\\s*((\\w+))", boost::regex_constants::optimize);
static const wregex sFunctionsDefinitionRegExW(L"^\\s*?((?!(return|if|else))\\w+)\\s+(\\w+)\\s*?\\(\\s*(([\\w\\s=\\-\\+\\*/\\.\\\"\\[\\]]*,?)*)\\)\\s*?;", boost::regex_constants::optimize);
static const wregex sFunctionsParamRegExW(L"(\\w+)\\s*(\\w+)\\s*(=\\s*(\\[[\\s\\d|\\w|\\-\\+\\*\\/\\.\\\",]*\\]|[\\w|\\d|.\\\"]*))?", boost::regex_constants::optimize);
static const wregex sConstantsRegExW(L"^\\s*?(\\w+)\\s*?(\\w+)\\s*?=?\\s*?([\\w|\\d|.\\-]*?)\\s*?;", boost::regex_constants::optimize);

using aregex = boost::basic_regex<char>;
using aregexmatch = boost::match_results<std::string::const_iterator>;
using asregex_iterator = boost::regex_iterator<std::string::const_iterator>;
using asregex_token_iterator = boost::regex_token_iterator<std::string::const_iterator>;
using amatch = boost::match_results<std::string>;

static const aregex sEngineStructRegExA("^\\s*?((?:#define))\\s*((?:ENGINE_STRUCTURE))_\\d*\\s*((\\w+))", boost::regex_constants::optimize);
static const aregex sFunctionsDefinitionRegExA("^\\s*?((?!(return|if|else))\\w+)\\s+(\\w+)\\s*?\\(\\s*(([\\w\\s=\\-\\+\\*/\\.\\\"\\[\\]]*,?)*)\\)\\s*?;", boost::regex_constants::optimize);
static const aregex sFunctionsParamRegExA("(\\w+)\\s*(\\w+)\\s*(=\\s*(\\[[\\s\\d|\\w|\\-\\+\\*\\/\\.\\\",]*\\]|[\\w|\\d|.\\\"]*))?", boost::regex_constants::optimize);
static const aregex sConstantsRegExA("^\\s*?(\\w+)\\s*?(\\w+)\\s*?=?\\s*?([\\w|\\d|.\\-]*?)\\s*?;", boost::regex_constants::optimize);

using wsconverter = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>;

// We setup constants according to the capture groups
constexpr const int preprocessor = 1;
constexpr const int engineWord = 2;
constexpr const int structName = 3;

constexpr const int functionType = 1;
constexpr const int functionName = 3;
constexpr const int functionParams = 4;

constexpr const int paramType = 1;
constexpr const int paramName = 2;
constexpr const int paramDefaultValue = 4;

constexpr const int constantType = 1;
constexpr const int constantName = 2;
constexpr const int constantValue = 3;

constexpr const int blockSize = 128 * 1024 + 4;


bool NWScriptParser::ParseFile(const generic_string& sFileName, ScriptParseResults& outParseResults)
{
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
	bool success = FileToBuffer(targetFileName, sFileContents);

	// Determines file encoding. We are using a maximum fixed block size to that.
	// uni8Bit is also returned by pure ASCII files.
	int encoding = Utf8_16_Read::determineEncoding((unsigned char*)sFileContents.c_str(), (blockSize > sFileContents.size()) ? sFileContents.size() : blockSize);
#ifdef USEADVANCEDENCODINGDETECTION
	// Cannot determine UTF-8 encoding.. try other method
	if (encoding == -1)
		encoding = detectCodepage(data, lenFile);
#endif

	if (encoding == uni8Bit || encoding == uni7Bit || encoding == uniCookie)
	{
		CreateNWScriptStructureA(sFileContents, outParseResults);
	}
	else if (encoding == uni16BE || encoding == uni16LE || encoding == uni16BE_NoBOM || encoding == uni16LE_NoBOM)
	{
		CreateNWScriptStructureW(sFileContents, outParseResults);
	}

	// Before returning to the caller, we sort the structure to better presentation and so AutoComplete can work properly.
	std::sort(outParseResults.Members.begin(), outParseResults.Members.end(), 
		[](ScriptMember a, ScriptMember b)	{  return a.sName < b.sName;});

	return true;

}

void NWScriptParser::resolveLinkFile(generic_string& linkFilePath)
{
	IShellLink* psl;
	WCHAR targetFilePath[MAX_PATH];
	WIN32_FIND_DATA wfd = {};

	HRESULT hres = CoInitialize(NULL);
	if (SUCCEEDED(hres))
	{
		hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
		if (SUCCEEDED(hres))
		{
			IPersistFile* ppf;
			hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
			if (SUCCEEDED(hres))
			{
				// Load the shortcut. 
				hres = ppf->Load(linkFilePath.c_str(), STGM_READ);
				if (SUCCEEDED(hres) && hres != S_FALSE)
				{
					// Resolve the link. 
					hres = psl->Resolve(NULL, 0);
					if (SUCCEEDED(hres) && hres != S_FALSE)
					{
						// Get the path to the link target. 
						hres = psl->GetPath(targetFilePath, MAX_PATH, (WIN32_FIND_DATA*)&wfd, SLGP_SHORTPATH);
						if (SUCCEEDED(hres) && hres != S_FALSE)
						{
							linkFilePath = targetFilePath;
						}
					}
				}
				ppf->Release();
			}
			psl->Release();
		}
		CoUninitialize();
	}
}

bool NWScriptParser::FileToBuffer(const generic_string& fileName, std::string& sContents)
{
	std::ifstream fileReadStream;

	fileReadStream.open(fileName.c_str(), std::ios::in | std::ios::binary);
	if (!fileReadStream.is_open())
	{
		return false;
	}

	fileReadStream.seekg(0, std::ios::end);
	sContents.resize(static_cast<std::size_t>(fileReadStream.tellg()));
	fileReadStream.seekg(0, std::ios::beg);
	size_t fileSize = sContents.size();
	fileReadStream.read(&sContents[0], fileSize);
	fileReadStream.close();

	return true;
}

void NWScriptParser::CreateNWScriptStructureA(const std::string& sFileContents, ScriptParseResults& outParseResults)
{
	// Since we are reading from 8-bit ASCII/UTF-8 and expecting TCHAR (UTF-16) result, we convert things here
	wsconverter converter;

	// Reserve a good amount of space since we're not doing dynamic allocations. 1 member per line is more than enough
	// Try standard EOL mode (\n). Then alternative (\r). Minimum to reserve is assumed to 1 member since at least 1 line 
	// will be processed by the RegEx.
	size_t lineCount = std::count(sFileContents.begin(), sFileContents.end(), '\n');
	if (lineCount == 0)
		lineCount = std::count(sFileContents.begin(), sFileContents.end(), '\r');

	outParseResults.Members.reserve(1 + lineCount);

	// Match subgroups for Engine Structures are: <preprocessor> [1], <EngineWord> [2], <name> [3]
	auto sBegin = asregex_iterator(sFileContents.begin(), sFileContents.end(), sEngineStructRegExA);
	auto sEnd = asregex_iterator();
	aregexmatch m, n;

	// Use Regular expressions to find members. First is Engine Structures
	// According to documentation, the first element of a match[] is the entire string.
	// Then they are segmented again in each expression subgroup.
	// https://www.cplusplus.com/reference/regex/match_results/

	int iCount = 0;
	for (asregex_iterator i = sBegin; i != sEnd; i++)
	{
		m = *i;
		// We are emplacing back MemberID = EngineStruct, sType = null, sName = <name>, sValue = nullptr, sParams = {empty}
		NWScriptParser::ScriptMember n;
		n.mID = MemberID::EngineStruct; n.sName = converter.from_bytes(m[structName].str().c_str());
		outParseResults.Members.emplace_back(n);
		iCount++;
	}
	outParseResults.EngineStructuresCount = iCount;

	// Match subgroups for Functions are: <type> [1], <name> [2], <params> [3]
	// <params> expands in <ptype> [1], <pname> [2], <pdefaultvalue> [4]
	sBegin = asregex_iterator(sFileContents.begin(), sFileContents.end(), sFunctionsDefinitionRegExA);
	sEnd = asregex_iterator();
	iCount = 0;
	for (asregex_iterator i = sBegin; i != sEnd; i++)
	{
		m = *i;
		std::vector<ScriptParamMember> sParams;

		std::string eMatch = m[functionParams].str().c_str();
		auto sBegin2 = asregex_iterator(eMatch.begin(), eMatch.end(), sFunctionsParamRegExA);
		auto sEnd2 = asregex_iterator();
		for (asregex_iterator j = sBegin2; j != sEnd2; j++)
		{
			n = *j;
			// Param Default value can be null
			ScriptParamMember s;
			s.sType = converter.from_bytes(n[paramType].str().c_str()); s.sName = converter.from_bytes(n[paramName].str().c_str());
			s.sDefaultValue = n.size() > 4 ? converter.from_bytes(n[paramDefaultValue].str().c_str()) : nullptr;
			sParams.push_back(s);
		}

		// We are pushing back MemberID = Function, sType = <type>, sName = <name>, sValue = nullptr, sParams = { <subquery> }
		// {<subquery>} = sType = <ptype>, sName = <pname>, sDefaultValue = <defaultvalue>
		NWScriptParser::ScriptMember Me;
		Me.mID = MemberID::Function; Me.sType = converter.from_bytes(m[functionType].str().c_str());
		Me.sName = converter.from_bytes(m[functionName].str().c_str()); Me.sParams = sParams;
		outParseResults.Members.emplace_back(Me);
		iCount++;
	}
	outParseResults.FunctionsCount = iCount;

	// Match subgroups for constants are:  <type> [1], <name> [2], <value> [3]
	sBegin = asregex_iterator(sFileContents.begin(), sFileContents.end(), sConstantsRegExA);
	sEnd = asregex_iterator();
	iCount = 0;
	for (asregex_iterator i = sBegin; i != sEnd; i++)
	{
		m = *i;

		// We are emplacing back MemberID = EngineStruct, sType = null, sName = <name>, sValue = nullptr, sParams = {empty}
		NWScriptParser::ScriptMember n;
		n.mID = MemberID::Constant; n.sType = converter.from_bytes(m[constantType].str().c_str()), n.sName = converter.from_bytes(m[constantName].str().c_str());
		n.sValue = converter.from_bytes(m[constantValue].str().c_str());
		outParseResults.Members.emplace_back(n);
		iCount++;
	}
	outParseResults.ConstantsCount = iCount;

	// Compacts memory back
	outParseResults.Members.shrink_to_fit();

	return;
}

void NWScriptParser::CreateNWScriptStructureW(const std::string& sFileContents, ScriptParseResults& outParseResults)
{
	// Since we're not re-encoding the file, we just assign pointers. Hopefully our encoding detector was right...
	std::wstring sNewFile;
	sNewFile.assign((wchar_t*)sFileContents.c_str(), sFileContents.size() / 2);

	// Reserve a good amount of space since we're not doing dynamic allocations. 1 member per line is more than enough
	// Try standard EOL mode (\n). Then alternative (\r). Minimum to reserve is assumed to 1 member since at least 1 line 
	// will be processed by the RegEx.
	size_t lineCount = std::count(sNewFile.begin(), sNewFile.end(), TEXT('\n'));
	if (lineCount == 0)
		lineCount = std::count(sNewFile.begin(), sNewFile.end(), TEXT('\r'));

	outParseResults.Members.reserve(1 + lineCount);

	// Match subgroups for Engine Structures are: <preprocessor> [1], <EngineWord> [2], <name> [3]
	auto sBegin = wsregex_iterator(sNewFile.begin(), sNewFile.end(), sEngineStructRegExW);
	auto sEnd = wsregex_iterator();
	wregexmatch m, n;

	int iCount = 0;
	for (wsregex_iterator i = sBegin; i != sEnd; i++)
	{
		m = *i;
		// We are emplacing back MemberID = EngineStruct, sType = null, sName = <name>, sValue = nullptr, sParams = {empty}
		NWScriptParser::ScriptMember n;
		n.mID = MemberID::EngineStruct; n.sName = m[structName].str().c_str();
		outParseResults.Members.emplace_back(n);
		iCount++;
	}
	outParseResults.EngineStructuresCount = iCount;

	// Match subgroups for Functions are: <type> [1], <name> [2], <params> [3]
	// <params> expands in <ptype> [1], <pname> [2], <pdefaultvalue> [4]
	sBegin = wsregex_iterator(sNewFile.begin(), sNewFile.end(), sFunctionsDefinitionRegExW);
	sEnd = wsregex_iterator();
	iCount = 0;
	for (wsregex_iterator i = sBegin; i != sEnd; i++)
	{
		m = *i;
		std::vector<ScriptParamMember> sParams;

		std::wstring eMatch = m[functionParams].str().c_str();
		auto sBegin2 = wsregex_iterator(eMatch.begin(), eMatch.end(), sFunctionsParamRegExW);
		auto sEnd2 = wsregex_iterator();
		for (wsregex_iterator j = sBegin2; j != sEnd2; j++)
		{
			n = *j;
			// Param Default value can be null
			ScriptParamMember s;
			s.sType = n[paramType].str().c_str(); s.sName = n[paramName].str().c_str();
			s.sDefaultValue = n.size() > 4 ? n[paramDefaultValue].str().c_str() : nullptr;
			sParams.push_back(s);
		}

		// We are pushing back MemberID = Function, sType = <type>, sName = <name>, sValue = nullptr, sParams = { <subquery> }
		// {<subquery>} = sType = <ptype>, sName = <pname>, sDefaultValue = <defaultvalue>
		NWScriptParser::ScriptMember Me;
		Me.mID = MemberID::Function; Me.sType = m[functionType].str().c_str();
		Me.sName = m[functionName].str().c_str(); Me.sParams = sParams;
		outParseResults.Members.emplace_back(Me);
		iCount++;
	}
	outParseResults.FunctionsCount = iCount;

	// Match subgroups for constants are:  <type> [1], <name> [2], <value> [3]
	sBegin = wsregex_iterator(sNewFile.begin(), sNewFile.end(), sConstantsRegExW);
	sEnd = wsregex_iterator();
	iCount = 0;
	for (wsregex_iterator i = sBegin; i != sEnd; i++)
	{
		m = *i;

		// We are emplacing back MemberID = EngineStruct, sType = null, sName = <name>, sValue = nullptr, sParams = {empty}
		NWScriptParser::ScriptMember n;
		n.mID = MemberID::Constant; n.sType = m[constantType].str().c_str(), n.sName = m[constantName].str().c_str();
		n.sValue = m[constantValue].str().c_str();
		outParseResults.Members.emplace_back(n);
		iCount++;
	}
	outParseResults.ConstantsCount = iCount;

	// Compacts memory back
	outParseResults.Members.shrink_to_fit();

	return;
}


#ifdef USEADVANCEDENCODINGDETECTION

/*
Requires uchardet library.
https://www.freedesktop.org/wiki/Software/uchardet/
This would be like overkill for a mere import... so left unused. :)
*/

int NWScriptParser::detectCodepage(char* buf, size_t len)
{
	int codepage = -1;
	uchardet_t ud = uchardet_new();
	uchardet_handle_data(ud, buf, len);
	uchardet_data_end(ud);
	const char* cs = uchardet_get_charset(ud);
	if (stricmp(cs, "TIS-620") != 0) // TIS-620 detection is disabled here because uchardet detects usually wrongly UTF-8 as TIS-620
		codepage = EncodingMapper::getInstance().getEncodingFromString(cs);
	uchardet_delete(ud);
	return codepage;
}

#endif
