/** @file AboutDialog.cpp
 * About Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include <Windows.h>
#include <Commctrl.h>
#include <sstream>

#include "jpcre2.hpp"

#include "PluginMain.h"
#include "AboutDialog.h"

#include "PluginControlsRC.h"

// Using method to read project version straight from binaries to avoid constant 
// recompilation of this file.
// #include "ProjectVersion.h"

#include "Common.h"
#include "VersionFromResource.h"


#define ABOUT_TEXT TEXT( \
"PLUGIN USAGE:\r\n\
-----------------\r\n\
- Select NWScript Language from Languages menu to enable syntax highlighting.\r\n\
\r\n\
- You may import a new nwscript.nss to replace the current engine definitions like constants, functions and engine structures and also enable Auto Complete functions to them. \
To do that, use the \"Import Definitions\" menu options. This will overwrite any previous engine definitions present on the plugin configuration. \
If you have any user-defined functions and constants previously imported or in use, don't worry, they will be preserved, as long as you did NOT \
put them manually inside the reserved sections of the XML configuration file. So I advise you to NEVER edit that file manually. Like, ever!\r\n\
\r\n\
- If for some reason the plugin colors break, use the \"Reset editor colors\" to fix. This will NOT erease any keyword or user-defined constants and functions \
in use.\r\n\
\r\n\
 - To enable .nss file compilation you first need to setup the environment and point to the proper Neverwinter installation folders, \
where the include files are present. Or you can make your own folder with the necessary files to compile a script. The nwscript.nss MUST \
be present in at least one of the defined paths.\r\n\
TIP: you may map a shortcut to the compilation menu to facilitate the process The default compilation shortcut is F9. \
If this is already taken, Notepad++ won't map it back to the plugin automatically. \r\n\
\r\n\
- If you want to change the Styles for NWScript in Notepad++ preferences, don't forget to first give write permissions on \
\"%PLUGINXMLFILE%\" to yourself, or else your changes won't be saved to the next session. This is a Notepad++ limitation.\r\n\
\r\n\
SELF-DIAGNOSTICS:\r\n\
-----------------\r\n\
  - Auto-Indentation: %NWSCRIPTINDENT%\r\n\
  - Dark Mode Theme:  %DARKTHEMESUPPORT%\r\n\
\r\n\
NERDY STATISTICS:\r\n\
-----------------\r\n\
  - Number of compilations attempts: %COMPILEATTEMPTS%\r\n\
  - Successful compilations:         %COMPILESUCCESSES%\r\n\
  - Failed compilations:             %COMPILESFAILED%\r\n\
\r\n\
  - NWscript defined structures:     %engineStructures%\r\n\
  - NWScript defined functions:      %engineFunctionCount%\r\n\
  - NWScript defined constants:      %engineConstants%\r\n\
  - User defined structures:         %userStructures%\r\n\
  - User defined functions:          %userFunctionCount%\r\n\
  - User defined constants:          %userConstants%\r\n\
\r\n\
")


using namespace NWScriptPlugin;

// Use one of these fonts to display text. Crescent order.
static const generic_string fontFamilies[] = {
	L"Cascadia Code", L"Cascadia Mono", L"Consolas", L"Droid Sans Mono", L"Inconsolata", L"Courier New" L"monospace", L"Monaco", L"Menlo", L"Fixedsys"
};

generic_string replaceAboutStrings(const generic_string& input, std::map<generic_string, generic_string>& replaceStrings)
{
	typedef jpcre2::select<wchar_t> pcre2;
	generic_string output = input;
	
	for (std::map<generic_string, generic_string>::const_iterator i = replaceStrings.begin(); i != replaceStrings.end(); i++)
	{
		output = pcre2::Regex(i->first.c_str()).replace(output, i->second.c_str(), "g");
	}

	return output;
}

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

			// Set user fonts. Try to keep same font for all.
			HFONT hTitleFont = NULL;
			int fontIndex = 0;
			while (fontIndex < std::size(fontFamilies) && !hTitleFont)
			{
				hTitleFont = ::CreateFont(16, 10, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontFamilies[fontIndex].c_str());
				if (!hTitleFont)
					fontIndex++;
			}

			HFONT hHomepageFont = CreateFont(14, 8, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, fontFamilies[fontIndex].c_str());

			if (hTitleFont)
				::SendMessage(GetDlgItem(_hSelf, IDC_LBLPLUGINNAME), WM_SETFONT, reinterpret_cast<WPARAM>(hTitleFont), 0);
			if (hHomepageFont)
				::SendMessage(GetDlgItem(_hSelf, IDC_LBLHOMEPAGE), WM_SETFONT, reinterpret_cast<WPARAM>(hHomepageFont), 0);

			::SetDlgItemText(_hSelf, IDC_LBLVERSION, reinterpret_cast<LPCWSTR>(sVersion.str().c_str()));
			::SetDlgItemText(_hSelf, IDC_TXTABOUT, replaceAboutStrings(ABOUT_TEXT, _replaceStrings).c_str());

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