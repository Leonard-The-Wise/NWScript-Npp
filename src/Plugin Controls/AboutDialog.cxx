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
#include "ProjectVersion.h"

#define ABOUT_TEXT \
TEXT("\
Usage:\r\n\
  - Select NWScript Language from Languages menu.\r\n\
  - You may import a new NWScript.nss and define your own Functions and Constants to the color syntax.\r\n\
\r\n\
Known Issues:\r\n\
  - Notepad++ Auto-Indentation function will conflict with the Plugin's Auto-Indentation. Please disable Notepad++ Auto-Indentation while editting NWScript files;\r\n\
  - Type-In kind of macros will break auto - indentation feature, please disable the Plugin \"Use Auto-Indentation\" feature while playing a macro;\r\n\
\r\n\
[Remark]: These are not bugs, but technical limitations for the Notepad++ plugin development plataform. I am currently trying to work with Notepad++ Team to fix these issues for future versions.")

using namespace NWScriptPlugin;
using tstringstream = std::basic_stringstream<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR>>;

intptr_t CALLBACK AboutDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		//Initializes Dialog textboxes and labels
		::SetDlgItemText(_hSelf, IDC_LBLVERSION, TEXT("(Version " VERSION_STRING_BUILDT ")"));
		::SetDlgItemText(_hSelf, IDC_TXTABOUT, ABOUT_TEXT);

		return TRUE;
		
	case WM_COMMAND:
		
		switch (wParam)
		{
		case IDCANCEL:
		case IDOK:
			display(false);
			return TRUE;
		}

	case WM_NOTIFY:
		
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
			}
		}
	}

	// Done processing messages
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