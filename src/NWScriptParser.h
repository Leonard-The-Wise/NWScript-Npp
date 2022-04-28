/** @file NWScriptParser.h
 * Parser for a NWScript.nss file
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/brief_syntax.h>
#include <bitsery/brief_syntax/string.h>
#include <bitsery/brief_syntax/vector.h>
#include <bitsery/brief_syntax/set.h>

#include "Common.h"

//some helper types
using Buffer = std::vector<uint8_t>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;

namespace NWScriptPlugin {

	class NWScriptParser {
	public:
		enum class MemberID { Unknown, EngineStruct, Constant, Function, Keyword };

		struct ScriptParamMember {
			friend class bitsery::Access;

			std::string sType;
			std::string sName;
			std::string sDefaultValue;
			bool operator==(const ScriptParamMember& other) const
			{
				return sType == other.sType && sName == other.sName && sDefaultValue == other.sDefaultValue;
			}

			bool operator<(const ScriptParamMember& other) const
			{
				if (sName < other.sName)
					return true;
				if (sName > other.sName)
					return false;
				if (sType < other.sType)
					return true;
				if (sType > other.sType)
					return false;
				// We don't check params[i].sDefaultValue for this...

				return false;
			}

		private:
			template <typename S>
			void serialize(S& s) {
				s(sType, sName, sDefaultValue);
			}
		};

		struct ScriptMember {
			friend class bitsery::Access;

			MemberID mID = MemberID::Unknown;
			std::string sType;
			std::string sName;
			std::string sValue;
			std::vector<ScriptParamMember> params;

			bool operator==(const ScriptMember& other) const
			{
				bool bEquals = mID == other.mID && sType == other.sType &&
					sName == other.sName && sValue == other.sValue;
				if (params.size() != other.params.size())
					return false;
				for (size_t i = 0; i < params.size(); i++)
				{
					if (params[i] != other.params[i])
						return false;
				}
				return bEquals;
			}

			bool operator<(const ScriptMember& other) const
			{
				if (sName < other.sName)
					return true;
				if (sName > other.sName)
					return false;

				if (sType < other.sType)
					return true;
				if (sType > other.sType)
					return false;

				if (sValue < other.sValue)
					return true;
				if (sValue > other.sValue)
					return false;

				if (params.size() < other.params.size())
					return true;
				if (params.size() > other.params.size())
					return false;

				for (size_t i = 0; i < params.size(); i++)
				{
					if (params[i].sType < other.params[i].sType)
						return true;
					if (params[i].sType > other.params[i].sType)
						return false;
					if (params[i].sName < other.params[i].sName)
						return true;
					if (params[i].sName > other.params[i].sName)
						return false;
					// We don't check params[i].sDefaultValue for this...
				}

				return false;
			}

		private:
			template <typename S>
			void serialize(S& s) {
				s(mID, sType, sName, sValue, params);
			}
		};

		struct ScriptParseResults
		{
			friend class bitsery::Access;

			size_t EngineStructuresCount = 0;
			size_t FunctionsCount = 0;
			size_t ConstantsCount = 0;
			size_t KeywordCount = 0;
			std::set<ScriptMember, std::less<ScriptMember>> Members;

			std::string MembersAsSpacedString(MemberID memberType) {
				std::string results;
				for (ScriptMember m : Members)
				{
					if (m.mID == memberType) {
						results.append(m.sName);results.append(" ");
					}
				}
				// Remove last space
				if (!results.empty())
					results.pop_back();
				return results;
			}

			void RecountStructs() {
				EngineStructuresCount = 0;
				FunctionsCount = 0;
				ConstantsCount = 0;
				KeywordCount = 0;

				for (ScriptMember s : Members)
				{
					if (s.mID == MemberID::Constant)
						ConstantsCount++;
					if (s.mID == MemberID::EngineStruct)
						EngineStructuresCount++;
					if (s.mID == MemberID::Function)
						FunctionsCount++;
					if (s.mID == MemberID::Keyword)
						KeywordCount++;
				}
			}

			void AddSpacedStringAsKeywords(const std::string& sKWArray);

			bool SerializeToFile(generic_string filePath);

			bool SerializeFromFile(generic_string filePath);

		private:
			template <typename S>
			void serialize(S& s) {
				s(EngineStructuresCount, FunctionsCount, ConstantsCount, KeywordCount, Members);
			}

		};

		explicit NWScriptParser(HWND MyParent) : _hWnd(MyParent) {}

		// Parse the Input file (ANSI or UNICODE) and if successful, returns a sorted members list from that file
		bool ParseFile(const generic_string& sFileName, ScriptParseResults& outParseResults);

		bool ParseBatch(const std::vector<generic_string>& sFilePaths, ScriptParseResults& outParseResults);

	private:
		HWND _hWnd;

		// Transforms a raw FileContent pointer into a ScriptParseResults list (for ASCII and UTF-8 based contents)
		void CreateNWScriptStructure(const std::string& sFileContents, ScriptParseResults& outParseResults);	
	};

};

