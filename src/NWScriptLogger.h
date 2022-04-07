/** @file NWScriptLogger.h
 * Replaces the default NWScript Compiler Library logger with something a little more... juicy.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <string>
#include <vector>

#include "Nsc.h"
#include "Common.h"

namespace NWScriptPlugin
{
	// Any class communicating with NscLib must implement IDebugTextOut.
	// We provide our own debug logger for that.
	class NWScriptLogger : public IDebugTextOut {

	public:

		NWScriptLogger() : _messageCallback(nullptr) {}

		enum class LogType {
			Critical, Error, Warning, Info, ConsoleMessage
		};

		struct CompilerMessage {
			LogType messageType = LogType::ConsoleMessage;
			generic_string messageText;
			generic_string messageCode;
			generic_string fileName;
			generic_string fileExt;
			generic_string lineNumber;
			filesystem::path filePath;

			bool operator==(const CompilerMessage& b) {
				return messageType == b.messageType && messageText == b.messageText &&
					messageCode == b.messageCode && fileName == b.fileName && fileExt == b.fileExt &&
					lineNumber == b.lineNumber;
			}
		};

		CompilerMessage operator[](size_t index) {
			return compilerMessages[index];
		}

		CompilerMessage getMessage(size_t index) {
			return compilerMessages[index];
		}

		filesystem::path getIncludeFile(size_t index) {
			return includeFiles[index];
		}

		void setProcessorString(const std::string& newValue) {
			processorContents.str(newValue);
		}

		const std::string getProcessorString() {
			return processorContents.str();
		}

		void clear() {
			compilerMessages.clear();
			includeFiles.clear();
			processorContents.str(std::string());
		}

		size_t logSize() {
			return compilerMessages.size();
		}

		size_t includeLogSize() {
			return includeFiles.size();
		}

		void setMessageCallback(void (*MessageCallback)(const CompilerMessage& message)) {
			_messageCallback = MessageCallback;
		}

		// Add log message to the stack
		void log(const generic_string& message, LogType type, const generic_string& messageCode, 
			const generic_string& fileName, const generic_string& fileExt, const generic_string& lineNumber);

		void log(const generic_string& message, LogType type, const generic_string& messageCode) {
			log(message, type, messageCode, TEXT(""), TEXT(""), TEXT(""));
		}

		void log(const generic_string& message, LogType type) {
			log(message, type, TEXT(""), TEXT(""), TEXT(""), TEXT(""));
		}

		void log(const std::string& message, LogType type, const std::string& messageCode, const std::string& fileName, 
			const std::string& fileExt, const std::string& lineNumber) {
			log(str2wstr(message.c_str()), type, str2wstr(messageCode.c_str()), str2wstr(fileName.c_str()), str2wstr(fileExt.c_str()), str2wstr(lineNumber.c_str()));
		}

		void log(const std::string& message, LogType type, const std::string& messageCode) {
			log(message, type, messageCode, "", "", "");
		}

		void log(const std::string& message, LogType type) {
			log(message, type, "", "", "", "");
		}

		// Implementation of IDebugTextOut: don't change
		virtual void WriteText(const char* fmt, ...);

		// Implementation of IDebugTextOut: don't change
		virtual void WriteTextV(const char* fmt, va_list ap);

	private:
		std::vector<CompilerMessage> compilerMessages;
		std::vector<filesystem::path> includeFiles;
		std::stringstream processorContents;

		void (*_messageCallback)(const CompilerMessage& message);

	};
}