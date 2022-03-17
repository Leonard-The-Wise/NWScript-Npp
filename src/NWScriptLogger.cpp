/** @file NWScriptLogger.cpp
 * Replaces the default NWScript Compiler Library logger with something a little more... juicy.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <stdarg.h>
//#include "jpcre2.hpp"

#include "NWScriptLogger.h"

#define PREPROCESSORPARSE R"((?:[\w\s\\.\-\(\):]+)(?:[\w\s\\.\-\(\):]+)\((?:\d+)\):\s(?:\w+):\s(?:NSC6022): Preprocessed: (.*))"
#define COMPILERREGEX R"((?<fileName>[\w\s\\.\-\(\):]+)\.(?<fileExt>[\w\s\\.\-\(\):]+)\((?<lineNumber>\d+)\):\s(?<type>\w+):\s(?<code>NSC\d+):\s(?<message>.+))"
#define GENERALMESSAGE R"(\s*(?<type>WARNING|ERROR|INFO)\s*:(?<message>.+))"
#define INCLUDESPARSEREGEX R"( ShowIncludes: Handled resource ([^\/]+)\/([^\\\n]+))"

typedef jpcre2::select<char> pcre2;

static const pcre2::Regex preprocessorRegex(PREPROCESSORPARSE, 0, jpcre2::JIT_COMPILE);
static const pcre2::Regex fileParsingMessageRegex(COMPILERREGEX, 0, jpcre2::JIT_COMPILE);
static const pcre2::Regex generalMessageRegex(GENERALMESSAGE, PCRE2_CASELESS, jpcre2::JIT_COMPILE);
static const pcre2::Regex includesRegex(INCLUDESPARSEREGEX, 0, jpcre2::JIT_COMPILE);

static pcre2::RegexMatch preprocessorParsing(&preprocessorRegex);
static pcre2::RegexMatch fileParsingMessage(&fileParsingMessageRegex);
static pcre2::RegexMatch generalMessage(&generalMessageRegex);
static pcre2::RegexMatch includeFile(&includesRegex);


using namespace NWScriptPlugin;

void NWScriptLogger::log(generic_string message, LogType type, generic_string messageCode, generic_string fileName,
	generic_string fileExt, generic_string lineNumber, bool merge)
{
	CompilerMessage newMessage = { type, messageCode, fileName, fileExt, lineNumber, message, merge };
	compilerMessages.push_back(newMessage);

	if (_messageCallback)
	{
		_messageCallback(newMessage);
	}
}

void NWScriptLogger::WriteText(const char* fmt, ...) {
	va_list params;
	va_start(params, fmt);
	WriteTextV(fmt, params);
	va_end(params);
}

void NWScriptLogger::WriteTextV(const char* fmt, va_list ap)
{
	pcre2::VecNas namedGroup;
	pcre2::VecNum numberedGroup;
	size_t matches;

	char buf[1024];	
	vsnprintf(buf, sizeof(buf), fmt, ap);

	// First deal with preprocessor's messages
	preprocessorParsing.setNumberedSubstringVector(&numberedGroup);
	preprocessorParsing.setSubject(buf);
	matches = preprocessorParsing.match();
	if (matches)
	{
		std::string matchdebug = numberedGroup[0][1];
		processorContents << numberedGroup[0][1] << "\n";
		return;
	}

	// Check for include files parsing
	includeFile.setNumberedSubstringVector(&numberedGroup);
	includeFile.setSubject(buf);
	matches = includeFile.match();
	if (matches)
	{
		includeFiles.push_back(properDirNameA(numberedGroup[0][1]) + "\\" + numberedGroup[0][2]);
		return;
	}

	// Check for compiler file parsing messages
	fileParsingMessage.setNamedSubstringVector(&namedGroup);
	fileParsingMessage.setSubject(buf);
	matches = fileParsingMessage.match();	
	if (matches)
	{
		log(namedGroup[0]["message"],
			(namedGroup[0]["type"] == "Error") ? LogType::Error : (namedGroup[0]["type"] == "Warning") ? LogType::Warning : LogType::Info,
			namedGroup[0]["code"], namedGroup[0]["fileName"], namedGroup[0]["fileExt"], namedGroup[0]["lineNumber"], false);

		return;
	}
	
	// Try again if nothing got first for general-purpose library messages
	generalMessage.setNamedSubstringVector(&namedGroup);
	generalMessage.setSubject(buf);
	matches = generalMessage.match();

	if (matches)
	{
		log(namedGroup[0]["message"], (namedGroup[0]["type"] == "ERROR") ? LogType::Error : (namedGroup[0]["type"] == "WARNING") ? LogType::Warning : LogType::Info);
		return;
	}

	// If everything else fails, write a simple log message
	log(buf, LogType::ConsoleMessage);
}

