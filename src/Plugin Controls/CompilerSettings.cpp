/** @file CompilerSettings.h
 * Compiler settings dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include <Windows.h>
#include <Commctrl.h>
#include <sstream>

#include "PluginMain.h"
#include "CompilerSettings.h"

#include "PluginControlsRC.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK CompilerSettingsDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		return TRUE;
	}

	case WM_COMMAND:
	{
		switch (wParam)
		{
		case IDCANCEL:
		case IDOK:
			display(false);
			destroy();
			return TRUE;
		}
	}
	}

	// Signals done processing messages
	return FALSE;
}

void CompilerSettingsDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_COMPILERSETTINGS);

	//Show and centralize
	goToCenter();
}