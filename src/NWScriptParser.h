/** @file NWScriptParser.h
 * Parser for a NWScript.nss file
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

//#define USEENCODINGDETECTION

#include <vector>
#include "Common.h"

//#pragma warning(push)
//#pragma warning(disable : 6387 26495)

namespace NWScriptPlugin {

	class NWScriptParser {
	public:
		enum class MemberID { Unknown, EngineStruct, Constant, Function };

		struct ScriptParamMember {
			generic_string sType;
			generic_string sName;
			generic_string sDefaultValue;
		};

		struct ScriptMember {
			MemberID mID = MemberID::Unknown;
			generic_string sType;
			generic_string sName;
			generic_string sValue;
			std::vector<ScriptParamMember> sParams;
		};

		struct ScriptParseResults
		{
			int EngineStructuresCount = 0;
			int FunctionsCount = 0;
			int ConstantsCount = 0;
			std::vector<ScriptMember> Members;
		};

		explicit NWScriptParser(HWND MyParent) : _hWnd(MyParent) {}

		// Parse the Input file (ANSI or UNICODE) and if successful, returns a sorted members list from that file
		bool ParseFile(const generic_string& sFileName, ScriptParseResults& outParseResults);

	private:
		HWND _hWnd;
		// Converts a File Link into an actual filename
		void resolveLinkFile(generic_string& linkFilePath);
		// Converts a file to a raw char* buffer and return the actual file size
		bool FileToBuffer(const generic_string& fileName, std::string& sContents);

		// Transforms a raw FileContent pointer into a ScriptParseResults list (for ASCII and UTF-8 based contents)
		void CreateNWScriptStructureA(const std::string& sFileContents, ScriptParseResults& outParseResults);
		// Transforms a raw FileContent pointer into a ScriptParseResults list (for UTF-16 and wchar_t* contents)
		void CreateNWScriptStructureW(const std::string& sFileContents, ScriptParseResults& outParseResults);

#ifdef USEENCODINGDETECTION
		int detectCodepage(char* buf, size_t len);
#endif

	};

}

//#pragma warning(pop)