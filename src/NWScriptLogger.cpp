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
#define GENERALMESSAGE R"(\s*(?<type>warning|error)\s*:(?<message>.))"

typedef jpcre2::select<char> pcre2;

static const pcre2::Regex compilerRegex(COMPILERREGEX, 0, jpcre2::JIT_COMPILE);
static const pcre2::Regex generalMessageRegex(GENERALMESSAGE, PCRE2_CASELESS, jpcre2::JIT_COMPILE);
static pcre2::RegexMatch compilerFunction(&compilerRegex);

using namespace NWScriptPlugin;

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

	char buf[8193];
	vsnprintf(buf, sizeof(buf), fmt, ap);
	
	compilerFunction.setNamedSubstringVector(&captureGroup);
	compilerFunction.setSubject(buf);
	matches = compilerFunction.match();

	CompilerMessage newMessage;

	if (matches)
	{
		newMessage.messageType = (captureGroup[0]["type"], "Error") ? LogType::Error :
			(captureGroup[0]["type"] == "Warning") ? LogType::Warning : LogType::Info;
		newMessage.fileName = str2wstr(captureGroup[0]["fileName"]);
		newMessage.fileExt = str2wstr(captureGroup[0]["fileExt"]);
		newMessage.lineNumber = str2wstr(captureGroup[0]["lineNumber"]);
		newMessage.messageCode = str2wstr(captureGroup[0]["code"]);
		newMessage.messageText = str2wstr(captureGroup[0]["message"]);
	}
	else
		newMessage.messageText = str2wstr(buf);

	compilerMessages.push_back(newMessage);

	if (_messageCallback)
	{
		_messageCallback(newMessage);
	}

}

void NWScriptLogger::log(generic_string message, LogType type, generic_string messageCode, generic_string fileName, 
	generic_string fileExt,	generic_string lineNumber, bool merge)
{
	CompilerMessage newMessage = { type, messageCode, fileName, fileExt, lineNumber, message, merge };
	compilerMessages.push_back(newMessage);
}