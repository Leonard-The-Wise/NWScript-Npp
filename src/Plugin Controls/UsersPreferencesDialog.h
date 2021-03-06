/** @file UsersPreferences.h
 * User's preferences dialog
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once


#include "Settings.h"
#include "StaticDialog.h"

namespace NWScriptPlugin {

	class UsersPreferencesDialog : public StaticDialog
	{
	public:
		UsersPreferencesDialog() = default;

		~UsersPreferencesDialog() {
			DeleteObject(_hWindowIcon);
		}

		void appendSettings(Settings* settings) {
			_settings = settings;
		}

		void setDarkModeInstalled(bool installed) {
			_darkModeInstalled = installed;
		}

		void doDialog();

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
		void keepSettings();
		Settings* _settings = nullptr;

		bool _darkModeInstalled = false;

		HICON _hWindowIcon = nullptr;
	};

}