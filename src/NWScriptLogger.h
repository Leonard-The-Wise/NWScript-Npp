/** @file NWScriptLogger.h
 * Replaces the default NWScript Compiler Library logger with something a little more... juicy.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <string>
#include <vector>

#include "TextOut.h"

namespace NWScriptPlugin
{
	// Any class communicating with NscLib must implement IDebugTextOut.
	// We provide our own debug logger for that.
	class NWScriptLogger : public IDebugTextOut {

	public:

		enum class LogType {
			Error, Warning, Info, Trace, Other
		};

		struct CompilerMessage {
			LogType messageType = LogType::Other;
			std::string fileName;
			std::string fileExt;
			std::string lineNumber;
			std::string messageCode;
			std::string messageText;
		};

		CompilerMessage operator[](size_t index) {
			return compilerMessages[index];
		}

		CompilerMessage getMessage(size_t index) {
			return compilerMessages[index];
		}

		void clear() {
			compilerMessages.clear();
		}

		size_t size() {
			return compilerMessages.size();
		}

		virtual void WriteText(const char* fmt, ...);

		virtual void WriteTextV(const char* fmt, va_list ap);

	private:
		std::vector<CompilerMessage> compilerMessages;

	};
}