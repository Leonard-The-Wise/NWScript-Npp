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

		void appendSettings(Settings* settings) {
			_settings = settings;
		}

		void doDialog();

		void SetDarkModeLegacyFunction(void (*UseDarkModeFunc)(bool UseDark)) {
			_UseDarkModeLegacy = UseDarkModeFunc;
		}

		void SetEnableDarkModeLegacy(bool isEnabled) {
			_UseDarkModeEnabledLegacy = isEnabled;
		}

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
		void keepSettings();

		void (*_UseDarkModeLegacy)(bool UseDark);
		bool _UseDarkModeEnabledLegacy = false;

		Settings* _settings = nullptr;
	};

}