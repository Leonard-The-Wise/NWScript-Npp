/** @file WarningDialog.cxx
 * Kind of a easter egg box...
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <Windows.h>
#include <Commctrl.h>
#include <sstream>

#include "WarningDialog.h"
#include "PluginMain.h"

#include "PluginControlsRC.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK WarningDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
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
				//case IDCANCEL:
				case IDOK:
					display(false);
					return TRUE;

				case IDC_CHKOK:
					Plugin::Instance().Settings().bAutoIndentationWarningAccepted = true;
					display(false);
					return TRUE;
			}
		}
	}

	// Signals done processing messages
	return FALSE;
}

void WarningDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_HEREBEDRAGONS);

	//Show and centralize
	goToCenter();
}