/** @file FileParseSummaryDialog.h
 * NWScript File Parsing Summary
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "StaticDialog.h"
#include "Common.h"

namespace NWScriptPlugin {

	class FileParseSummaryDialog : public StaticDialog
	{
	public:
		FileParseSummaryDialog() = default;

		~FileParseSummaryDialog() {
			DeleteObject(_hFile);
		}

		void setOkDialogCallback(void (*OkDialogCallback)(HRESULT decision)) {
			_okDialogCallback = OkDialogCallback;
		}
		void setEngineStructuresCount(int engineStructuresCount) {
			_engineStructuresCount = std::to_wstring(engineStructuresCount);
		}
		void setFunctionDefinitionsCount(int functionDefinitionCount) {
			_functionsDefinitionCount = std::to_wstring(functionDefinitionCount);
		}
		void setConstantsCount(int constantsCount) {
			_constantsCount = std::to_wstring(constantsCount);
		}
		void doDialog();

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
		generic_string _engineStructuresCount;
		generic_string _functionsDefinitionCount;
		generic_string _constantsCount;

		void setLogo();

		void (*_okDialogCallback)(HRESULT decision) = nullptr;

		HBITMAP _hFile;
	};
}