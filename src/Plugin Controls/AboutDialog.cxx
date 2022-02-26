/** @file AboutDialog.cxx
 * About Dialog Box
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <Windows.h>
#include <Commctrl.h>
#include <sstream>

#include "AboutDialog.h"
#include "PluginMain.h"

#include "PluginControlsRC.h"

// Using method to read project version straight from binaries to avoid constant 
// recompilation of this file.
// #include "ProjectVersion.h"

#include "VersionFromResource.h"


#define ABOUT_TEXT \
TEXT("\
USAGE:\r\n\
  - Select NWScript Language from Languages menu.\r\n\
  - You may import a new NWScript.nss and define your own Functions and Constants to the color syntax.\r\n\
  - To customize colors go to Settings -> Style Configurator -> NWScript files.\r\n\
  - To be able to compile .nss files you first need to setup the environment and point to the proper Neverwinter folders where the include files are present (tip: map shortcuts to the compilation menu to facilitate the process). \
\r\n\r\n")


using namespace NWScriptPlugin;
typedef std::basic_string<TCHAR> generic_string;
typedef std::basic_stringstream<TCHAR> generic_stringstream;

intptr_t CALLBACK AboutDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			// Get version from module's binary file
			DllVersionInfo versionInfo = GetVersionFromResource(Plugin::Instance().DllHModule());
			generic_stringstream sVersion = {};
			sVersion << "Version " << versionInfo.dwLeftMost << "." << versionInfo.dwSecondLeft << "." <<
				versionInfo.dwSecondRight << " (build " << versionInfo.dwRightMost << ")";

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