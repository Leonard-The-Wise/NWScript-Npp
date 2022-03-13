/** @file CompilerSettings.h
 * Compiler settings dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "ModalDialog.h"
#include "CompilerSettings.h"

namespace NWScriptPlugin {

	class CompilerSettingsDialog : public ModalDialog
	{
	public:
		CompilerSettingsDialog() = default;

		void appendSettings(Settings* settings)	{
			_settings = settings;
		}

		virtual INT_PTR doDialog();

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
		bool keepSettings();
		Settings* _settings = nullptr;
	};

}