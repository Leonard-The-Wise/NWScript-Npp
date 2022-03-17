/** @file AboutDialog.cpp
 * About Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include <sstream>
//
//#include "jpcre2.hpp"

//#include "PluginMain.h"
#include "AboutDialog.h"

#include "PluginControlsRC.h"

// Using method to read project version straight from binaries to avoid constant 
// recompilation of this file .
// #include "ProjectVersion.h"
#include "VersionInfo.h"


#define ABOUT_TEXT TEXT( \
" ** For a more detailed and formated version of the help text, please visit the web page on the \"Online Help\" menu. **\r\n\
 ** Diagnostics information and some use statistics are avaliable at the end of this text. **\r\n\
\r\n\
PLUGIN USAGE:\r\n\
-----------------\r\n\
• Editor Syntax Highlighting:\r\n\
   - Select NWScript Language from Languages menu to enable syntax highlighting.\r\n\
\r\n\
• Using Auto-Indentation:\r\n\
   - For Notepad++ 8.3.2 and bellow, chose the option \"Use Auto-Indentation\" to enable the plugin's built-in auto-indentation support. \
You'll need to disable Notepad++ Auto-Indentation in user preferences to avoid any conflicts. This feature is automatic on Notepad++ 8.3.3 \
and beyond so this menu option won't show anymore for users with up-to-date versions.\r\n\
\r\n\
• Menu - \"Compile script\":\r\n\
   - Compiles the current opened document into the \"Output Directory\" set in \"Compiler Settings\".\r\n\
\r\n\
• Menu - \"Disassemble file\":\r\n\
   - Disassembles a compiled NWscript file from the disk and put results into the \"Output Directory\" set in \"Compiler Settings\".\r\n\
\r\n\
• Menu - \"Batch processing\":\r\n\
   - Opens the Batch-processing dialog box.\r\n\
\r\n\
• Menu - \"Run last batch\":\r\n\
   - Runs the last successful batch operation in this session.\r\n\
\r\n\
• Menu - \"Fetch preprocessor output\":\r\n\
   - Runs a compile preprocessing phase on current script and display the results in a new document for the user. \
Useful to view what final text the compiler will ACTUALLY use to compile the script.\r\n\
\r\n\
• Menu - \"View Script Dependencies\":\r\n\
   - Parse the script file's dependencies and display to the user as a new human-readable document.\r\n\
\r\n\
• Menu - \"Compiler settings\":\r\n\
   - Opens the compiler settings.\r\n\
\r\n\
• Menu - \"User's Preferences\":\r\n\
   - Opens the user's preferences dialog.\r\n\
\r\n\
• Menu - \"Install Dark Theme\":\r\n\
   - Installs Dark Theme support if not already present. (When installation is detected, this option won't show up).\r\n\
\r\n\
• Menu - \"Import NWScript definitions\":\r\n\
   - With this option, you may import a new \"nwscript.nss\" to replace the current engine definitions like constants, functions and engine \
structures to use with syntax coloring and highlighting and also this enables the Auto Complete functions to them. This will overwrite any \
previous engine definitions present on the plugin configuration.\r\n\
\r\n\
• Menu - \"Import user-defined tokens\":\r\n\
   - With this option, you may import new user-defined functions and constants from any .nss file to enable color-syntax highlighting and auto-completion to them. \
Please notice that only function DECLARATIONS and GLOBAL CONSTANTS will be imported in this process. \
And if you have any user-defined functions and constants previously imported or in use, don't worry, they will be preserved, as long as you did NOT \
put them manually inside the reserved sections of the XML configuration file. So I advise you to NEVER edit that file manually. Like, ever!\r\n\
\r\n\
• Menu - \"Reset user-defined tokens\":\r\n\
   - This will clear ANY user-defined functions and constants previously imported to the plugin's configurations.\r\n\
\r\n\
• Menu - \"Reset editor colors\":\r\n\
   - This will reset all editor color styles to the default values - eihter for light and dark themes. No function or constants definitions will be ereased in the process.\r\n\
\r\n\
• Menu - \"About me\":\r\n\
   - Self reference ®. :)\r\n\
\r\n\
\r\n\
REMARKS:\r\n\
-----------------\r\n\
• If you noticed the SHIELD icons near some menu options, that's because if you want to use that option you must either run Notepad++ with\
administrator privileges OR you may also manually provide write permissions to the option's required files (you'll be notified of which file you\
need to provide permissions to for each option.\r\n\
\r\n\
\r\n\
EXTRA COPYRIGHT INFO:\r\n\
---------------\r\n\
\r\n\
Copyright notices for embbeded version of the NWScript Compiler Library:\r\n\
\r\n\
  Portions Copyright (C) 2008-2015 Skywing\r\n\
  Portions copyright (C) 2002-2003, Edward T. Smith\r\n\
  Portions copyright (C) 2003, The Open Knights Consortium\r\n\
  Adapted for Neverwinter Nights Enhanced Edition and cross platform use by: Glorwinger and Jakkn\r\n\
  Readapted for Windows GUI applications by: Leonard-The-Wise\r\n\
\r\n\
\r\n\
SELF-DIAGNOSTICS:\r\n\
-----------------\r\n\
  - Auto-Indentation: %NWSCRIPTINDENT%\r\n\
  - Dark Mode Theme:  %DARKTHEMESUPPORT%\r\n\
\r\n\
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

intptr_t CALLBACK AboutDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			// Get version from module's binary file
			VersionInfo versionInfo = VersionInfo::getLocalVersion();
			generic_stringstream sVersion = {};
			sVersion << "Version " << versionInfo.shortString().c_str() << " (build " << versionInfo.build() << ")";

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
			::SetDlgItemText(_hSelf, IDC_TXTABOUT, replaceStringsW(ABOUT_TEXT, _replaceStrings).c_str());
			::SetDlgItemText(_hSelf, IDC_LNKHOMEPAGE, (TEXT("<a href=\"") + _homePath + TEXT("\">") + _homePath + TEXT("</a>")).c_str());

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