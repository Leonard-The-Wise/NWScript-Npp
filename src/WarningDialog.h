/** @file Warning.h
 * Warning Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "Settings.h"
#include "StaticDialog.h"

namespace NWScriptPlugin {

	class WarningDialog : public StaticDialog
	{
	public:
		WarningDialog() = default;

		void appendSettings(Settings* settings) {
			_settings = settings;
		}

		void doDialog();

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
		Settings* _settings = nullptr;
	};

}