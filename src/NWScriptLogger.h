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
			generic_string messageCode;
			generic_string fileName;
			generic_string fileExt;
			generic_string lineNumber;
			generic_string messageText;
			bool merge = false;
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

		void setMessageCallback(void (*MessageCallback)(CompilerMessage& message)) {
			_messageCallback = MessageCallback;
		}

		// Add log message to the stack
		void log(generic_string message, LogType type, generic_string messageCode, generic_string fileName, generic_string fileExt, 
			generic_string lineNumber, bool merge);

		void log(generic_string message, LogType type, generic_string messageCode, generic_string fileName,
			generic_string fileExt, generic_string lineNumber) {
			log(message, type, messageCode, fileName, fileExt, lineNumber, false);
		}

		void log(generic_string message, LogType type, generic_string messageCode, bool merge) {
			log(message, type, messageCode, TEXT(""), TEXT(""), TEXT(""), merge);
		}

		void log(generic_string message, LogType type, generic_string messageCode) {
			log(message, type, messageCode, TEXT(""), TEXT(""), TEXT(""), false);
		}

		void log(generic_string message, LogType type, bool merge) {
			log(message, type, TEXT(""), TEXT(""), TEXT(""), TEXT(""), merge);
		}

		void log(generic_string message, LogType type) {
			log(message, type, TEXT(""), TEXT(""), TEXT(""), TEXT(""), false);
		}

		void log(std::string message, LogType type, std::string messageCode, std::string fileName, std::string fileExt,
			std::string lineNumber, bool merge) {
			log(str2wstr(message.c_str()), type, str2wstr(messageCode.c_str()), str2wstr(fileName.c_str()), str2wstr(fileExt.c_str()), str2wstr(lineNumber.c_str()), merge);
		}

		void log(std::string message, LogType type, std::string messageCode, std::string fileName,
			std::string fileExt, std::string lineNumber) {
			log(message, type, messageCode, fileName, fileExt, lineNumber, false);
		}

		void log(std::string message, LogType type, std::string messageCode, bool merge) {
			log(message, type, messageCode, "", "", "", merge);
		}

		void log(std::string message, LogType type, std::string messageCode) {
			log(message, type, messageCode, "", "", "", false);
		}

		void log(std::string message, LogType type, bool merge) {
			log(message, type, "", "", "", "", merge);
		}

		void log(std::string message, LogType type) {
			log(message, type, "", "", "", "", false);
		}

		// Implementation of IDebugTextOut: don't change
		virtual void WriteText(const char* fmt, ...);

		// Implementation of IDebugTextOut: don't change
		virtual void WriteTextV(const char* fmt, va_list ap);

	private:
		std::vector<CompilerMessage> compilerMessages;
		void (*_messageCallback)(CompilerMessage& message);

	};
}