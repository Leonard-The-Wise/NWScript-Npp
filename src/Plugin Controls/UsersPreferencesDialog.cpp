/** @file UsersPreferences.cpp
 * User's preferences dialog
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.



#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include <sstream>

//#include "PluginMain.h"
#include "UsersPreferencesDialog.h"

#include "PluginControlsRC.h"
#include "PluginDarkMode.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK UsersPreferencesDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			CheckDlgButton(_hSelf, IDC_CHKAUTOOPENDISASSEMBLED, _settings->autoDisplayDisassembled ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(_hSelf, IDC_CHKAUTOOPENDEBUGSYMBOLS, _settings->autoDisplayDebugSymbols ? BST_CHECKED : BST_UNCHECKED);

			if (_darkModeInstalled)
				CheckDlgButton(_hSelf, IDC_CHKAUTOINSTALLDARKTHEME, _settings->autoInstallDarkTheme ? BST_CHECKED : BST_UNCHECKED);
			else
			{
				EnableWindow(GetDlgItem(_hSelf, IDC_CHKAUTOINSTALLDARKTHEME), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_LBLDARKMODEEXPLAIN), false);
				SetWindowText(GetDlgItem(_hSelf, IDC_LBLDARKMODEEXPLAIN), TEXT("(To use this option, first install Dark Theme on menu options)"));
			}

			_dpiManager.resizeControl(_hSelf);
			_dpiManager.resizeChildren(_hSelf, true);

			PluginDarkMode::autoSetupWindowAndChildren(_hSelf);

			// Window icon
			_hWindowIcon = loadSVGFromResourceIcon(_hInst, IDI_SHOWASSIGNEDCONFIGURATION, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16));
			::SendMessage(_hSelf, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(_hWindowIcon));

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
			case IDCANCEL:
			case IDOK:
				if (wParam == IDOK)
					keepSettings();
				display(false);
				destroy();
				return TRUE;
			}
			break;
		}
	}

	// Signals done processing messages
	return FALSE;
}

void UsersPreferencesDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_USERSPREFERENCES);

	//Show and centralize
	goToCenter();
}


void UsersPreferencesDialog::keepSettings()
{
	_settings->autoDisplayDisassembled = IsDlgButtonChecked(_hSelf, IDC_CHKAUTOOPENDISASSEMBLED);
	_settings->autoDisplayDebugSymbols = IsDlgButtonChecked(_hSelf, IDC_CHKAUTOOPENDEBUGSYMBOLS);

	if (_darkModeInstalled)
		_settings->autoInstallDarkTheme = IsDlgButtonChecked(_hSelf, IDC_CHKAUTOINSTALLDARKTHEME);
}

