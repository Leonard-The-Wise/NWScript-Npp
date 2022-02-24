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

#define ABOUT_TEXT_ISSUES \
TEXT("KNOWN ISSUES (for this version of Notepad++ and bellow):\r\n\
  - Notepad++ Auto-Indentation function will conflict with the Plugin's Auto-Indentation. Please disable Notepad++ Auto-Indentation while editting NWScript files;\r\n\
  - Type-In kind of macros will also break auto-indentation feature, please disable the Plugin \"Use Auto-Indentation\" feature while playing a macro;\r\n\
\r\n\
[Remark]: These are not bugs, but technical limitations for the Notepad++ of versions 8.3.x and bellow. I am currently working with Notepad++ \
Team to fix these issues for future versions, so if this message still appears for you, consider upgrading Notepad++. It will auto-disappear upon having the necessary funcionality.")

#define ABOUT_TEXT_NEW \
TEXT("FEATURES:\r\n\
  - Auto-Indentation feature integrated into Notepad++. You can now turn Notepad++'s Auto-Indentation on while editing .nss scripts.")


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
			generic_stringstream sVersion;
			sVersion << "(Version " << versionInfo.dwLeftMost << "." << versionInfo.dwSecondLeft << "." <<
				versionInfo.dwSecondRight << " [build " << versionInfo.dwRightMost << "])";

			// Build the "About" text
			generic_stringstream sTxtAbout;
			sTxtAbout << ABOUT_TEXT;

			// For older notepad, display the Issues warning.
			if (Plugin::Instance().NeedsPluginAutoIndent())
				sTxtAbout << ABOUT_TEXT_ISSUES;
			else
				sTxtAbout << ABOUT_TEXT_NEW;

			::SetDlgItemText(_hSelf, IDC_LBLVERSION, reinterpret_cast<LPCWSTR>(sVersion.str().c_str()));
			::SetDlgItemText(_hSelf, IDC_TXTABOUT, reinterpret_cast<LPCWSTR>(sTxtAbout.str().c_str()));

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