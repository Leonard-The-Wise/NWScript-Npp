/** @file NWScriptParser.h
 * Parser for a NWScript.nss file
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "Common.h"

namespace NWScriptPlugin {

	class NWScriptParser {
	public:
		enum class MemberID { Unknown, EngineStruct, Constant, Function, Keyword };

		struct ScriptParamMember {
			generic_string sType;
			generic_string sName;
			generic_string sDefaultValue;
			bool operator==(const ScriptParamMember& other)
			{
				return sType == other.sType && sName == other.sName && sDefaultValue == other.sDefaultValue;
			}
		};

		struct ScriptMember {
			MemberID mID = MemberID::Unknown;
			generic_string sType;
			generic_string sName;
			generic_string sValue;
			std::vector<ScriptParamMember> params;

			bool operator==(const ScriptMember& other)
			{
				bool bEquals = mID == other.mID && sType == other.sType &&
					sName == other.sName && sValue == other.sValue;
				if (params.size() != other.params.size())
					return false;
				for (int i = 0; i < params.size(); i++)
				{
					if (params[i] != other.params[i])
						return false;
				}
				return bEquals;
			}
		};

		struct ScriptParseResults
		{
			int EngineStructuresCount = 0;
			int FunctionsCount = 0;
			int ConstantsCount = 0;
			int KeywordCount = 0;
			std::vector<ScriptMember> Members;

			generic_string MembersAsSpacedString(MemberID memberType) {
				generic_string results;
				for (ScriptMember m : Members)
				{
					if (m.mID == memberType) {
						results.append(m.sName);results.append(TEXT(" "));
					}
				}
				// Remove last space
				if (!results.empty())
					results.pop_back();
				return results;
			}

			void AddSpacedStringAsMember(const generic_string& sKWArray, MemberID memberID);

			bool SerializeToFile(generic_string filePath);

			bool SerializeFromFile(generic_string filePath);

			void Sort();
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

//#pragma warning(pop)