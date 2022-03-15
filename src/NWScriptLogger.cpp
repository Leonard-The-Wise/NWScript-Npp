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

#define COMPILERREGEX R"((?<fileName>[a-zA-Z0-9\s_\\.\-\(\):]+)\.(?<fileExt>[a-zA-Z0-9\s_\\.\-\(\):]+)\((?<lineNumber>\d+)\):\s(?<type>\w+):\s(?<code>NSC\d+):\s(?<message>.+))"
#define GENERALMESSAGE R"(\s*(?<type>WARNING|ERROR|INFO)\s*:(?<message>.))"

typedef jpcre2::select<char> pcre2;

static const pcre2::Regex fileParsingMessageRegex(COMPILERREGEX, 0, jpcre2::JIT_COMPILE);
static const pcre2::Regex generalMessageRegex(GENERALMESSAGE, PCRE2_CASELESS, jpcre2::JIT_COMPILE);

static pcre2::RegexMatch fileParsingMessage(&fileParsingMessageRegex);
static pcre2::RegexMatch generalMessage(&generalMessageRegex);


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
	pcre2::VecNas captureGroup;
	size_t matches;

	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, ap);
	
	fileParsingMessage.setNamedSubstringVector(&captureGroup);
	fileParsingMessage.setSubject(buf);
	matches = fileParsingMessage.match();
	
	if (matches)
	{
		log(captureGroup[0]["message"],
			(captureGroup[0]["type"], "Error") ? LogType::Error : (captureGroup[0]["type"] == "Warning") ? LogType::Warning : LogType::Info,
			captureGroup[0]["code"], captureGroup[0]["fileName"], captureGroup[0]["fileExt"], captureGroup[0]["lineNumber"], false);

		return;
	}
	
	// Try again if nothing got first
	generalMessage.setNamedSubstringVector(&captureGroup);
	generalMessage.setSubject(buf);
	matches = generalMessage.match();

	if (matches)
	{
		log(captureGroup[0]["message"], (captureGroup[0]["type"], "ERROR") ? LogType::Error : (captureGroup[0]["type"] == "WARNING") ? LogType::Warning : LogType::Info);
		return;
	}

	// If everything else fails, write a simple log message
	log(buf, LogType::ConsoleMessage);
}

