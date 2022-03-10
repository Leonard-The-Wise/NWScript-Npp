/** @file AboutDialog.cpp
 * About Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <Windows.h>
#include <Commctrl.h>
#include <sstream>

#include "PluginMain.h"
#include "AboutDialog.h"

#include "PluginControlsRC.h"

// Using method to read project version straight from binaries to avoid constant 
// recompilation of this file.
// #include "ProjectVersion.h"

#include "Common.h"
#include "VersionFromResource.h"


#define ABOUT_TEXT \
TEXT("\
USAGE:\r\n\
  - Select NWScript Language from Languages menu.\r\n\
  - You may import a new NWScript.nss and define your own Functions and Constants to the color syntax.\r\n\
  - To be able to compile .nss files you first need to setup \
the environment and point to the proper Neverwinter folders where the include files are present \
(tip: map shortcuts to the compilation menu to facilitate the process).\r\n \
  - If you want to change the Styles for NWScript in Notepad++ preferences, don't forget to first give write permissions on $FILE$, or else \
your changes won't be saved to the next session.\r\n")


using namespace NWScriptPlugin;

intptr_t CALLBACK AboutDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			// Get version from module's binary file
			DllVersion versionInfo = GetVersionFromResource(Plugin::Instance().DllHModule());
			generic_stringstream sVersion = {};
			sVersion << "Version " << versionInfo.wLeftMost << "." << versionInfo.wSecondLeft << "." <<
				versionInfo.wSecondRight << " (build " << versionInfo.wRightMost << ")";
			generic_string usageText = ABOUT_TEXT;
			::SetDlgItemText(_hSelf, IDC_LBLVERSION, reinterpret_cast<LPCWSTR>(sVersion.str().c_str()));
			::SetDlgItemText(_hSelf, IDC_TXTABOUT, ABOUT_TEXT);

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
		case WM_NOTIFY:
		{
			if (wParam == IDC_LNKHOMEPAGE)
			{
				NMHDR* nmhdr = reinterpret_cast<NMHDR*>(lParam);
				switch (nmhdr->code)
				{
				case NM_CLICK:
				case NM_RETURN:
					PNMLINK pNMLink = (PNMLINK)lParam;
					LITEM link = pNMLink->item;

					ShellExecute(NULL, L"open", link.szUrl, NULL, NULL, SW_SHOW);
					return TRUE;
				}
			}
		}
	}

	// Signals done processing messages
	return FALSE;
}

void AboutDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_ABOUT);

	//Show and centralize
	goToCenter();
}