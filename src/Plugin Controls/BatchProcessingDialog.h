/** @file BatchProcessing.h
 * Batch file processing dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "StaticDialog.h"

#include "Common.h"


namespace NWScriptPlugin {

	class BatchProcessingDialog : public StaticDialog
	{
	public:
		BatchProcessingDialog() = default;

		~BatchProcessingDialog() {
			DeleteObject(_hWindowIcon);
		}

		void setOkDialogCallback(void (*OkDialogCallback)(HRESULT decision)) {
			_okDialogCallback = OkDialogCallback;
		}

		void appendSettings(Settings* settings) {
			_settings = settings;
		}

		void doDialog();

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
		bool keepSettings();
		generic_string _tmpFiltersCompile;
		generic_string _tmpFiltersDisasm;
		Settings* _settings = nullptr;

		HICON _hWindowIcon = nullptr;

		void (*_okDialogCallback)(HRESULT decision) = nullptr;
	};

}