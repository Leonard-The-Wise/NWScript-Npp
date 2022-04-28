/** @file CompilerSettings.h
 * Compiler settings dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <Windows.h>
//#include <windowsx.h>
//#include <Commctrl.h>
//#include <sstream>

#include "Nsc.h"
#include "CompilerSettingsDialog.h"

#include "PluginControlsRC.h"
#include "PluginDarkMode.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK CompilerSettingsDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			_dpiManager.resizeControl(_hSelf);
			_dpiManager.resizeChildren(_hSelf, true);

			PluginDarkMode::autoSetupWindowAndChildren(_hSelf);

			Settings& myset = *_settings;
			if (myset.neverwinterInstallChoice == 0)
			{
				::CheckRadioButton(_hSelf, IDC_USENWN1, IDC_USENWN2, IDC_USENWN1);
				EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN2INSTALL), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN2INSTALL), false);
			}
			else
			{
				::CheckRadioButton(_hSelf, IDC_USENWN1, IDC_USENWN2, IDC_USENWN2);
				EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN1INSTALL), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN1INSTALL), false);
			}

			SetDlgItemText(_hSelf, IDC_TXTNWN1INSTALL, myset.neverwinterOneInstallDir.c_str());
			SetDlgItemText(_hSelf, IDC_TXTNWN2INSTALL, myset.neverwinterTwoInstallDir.c_str());

			CheckDlgButton(_hSelf, IDC_CHKIGNOREINSTALLPATHS, myset.ignoreInstallPaths ? BST_CHECKED : BST_UNCHECKED);
			if (myset.ignoreInstallPaths)
			{
				EnableWindow(GetDlgItem(_hSelf, IDC_USENWN1), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_USENWN2), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN1INSTALL), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN2INSTALL), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN1INSTALL), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN2INSTALL), false);
			}

			for (generic_string s : myset.getIncludeDirsV())
			{
				ListBox_AddString(GetDlgItem(_hSelf, IDC_LSTADDPATH), s.c_str());
			}

			CheckDlgButton(_hSelf, IDC_CHKCOMPOPTIMIZE, myset.optimizeScript);
			CheckDlgButton(_hSelf, IDC_CHKNONBIOWAREXTENSIONS, myset.useNonBiowareExtenstions);
			CheckDlgButton(_hSelf, IDC_CHKCOMPNDBSYMBOLS, myset.generateSymbols);
			CheckDlgButton(_hSelf, IDC_CHKCOMPSTRICTMODE, myset.compilerFlags & NscCompilerFlag_StrictModeEnabled);
			CheckDlgButton(_hSelf, IDC_CHKCOMPMAKEFILE, myset.compilerFlags & NscCompilerFlag_GenerateMakeDeps);
			CheckDlgButton(_hSelf, IDC_CHKCOMPDISABLESLASHPARSE, myset.compilerFlags & NscCompilerFlag_DisableDoubleQuote);

			ComboBox_AddString(GetDlgItem(_hSelf, IDC_CBOTARGETVERSION), TEXT("174+"));
			ComboBox_AddString(GetDlgItem(_hSelf, IDC_CBOTARGETVERSION), TEXT("169"));
			ComboBox_SetCurSel(GetDlgItem(_hSelf, IDC_CBOTARGETVERSION), myset.compileVersion == 174 ? 0 : 1);

			SetDlgItemText(_hSelf, IDC_TXTOUTPUTDIR, myset.outputCompileDir.c_str());
			CheckDlgButton(_hSelf, IDC_CHKOUTPUTDIR, myset.useScriptPathToCompile);
			if (myset.useScriptPathToCompile)
			{
				EnableWindow(GetDlgItem(_hSelf, IDC_TXTOUTPUTDIR), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_BTOUTPUTDIR), false);
			}

			// Window icon
			_hWindowIcon = loadSVGFromResourceIcon(_hInst, IDI_SETTINGSGROUP, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16));
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
					{
						if (!keepSettings())
							return FALSE;
					}
					EndDialog(_hSelf, wParam);
					break;

				case IDC_USENWN1:
				{
					EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN1INSTALL), true);
					EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN1INSTALL), true);
					EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN2INSTALL), false);
					EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN2INSTALL), false);
					return FALSE;
				}

				case IDC_USENWN2:
				{
					EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN1INSTALL), false);
					EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN1INSTALL), false);
					EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN2INSTALL), true);
					EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN2INSTALL), true);
					return FALSE;
				}

				case IDC_CHKIGNOREINSTALLPATHS:
				{
					if (IsDlgButtonChecked(_hSelf, IDC_CHKIGNOREINSTALLPATHS))
					{
						EnableWindow(GetDlgItem(_hSelf, IDC_USENWN1), false);
						EnableWindow(GetDlgItem(_hSelf, IDC_USENWN2), false);
						EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN1INSTALL), false);
						EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN2INSTALL), false);
						EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN1INSTALL), false);
						EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN2INSTALL), false);
					}
					else
					{
						EnableWindow(GetDlgItem(_hSelf, IDC_USENWN1), true);
						EnableWindow(GetDlgItem(_hSelf, IDC_USENWN2), true);
						EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN1INSTALL), true);
						EnableWindow(GetDlgItem(_hSelf, IDC_TXTNWN2INSTALL), true);
						EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN1INSTALL), true);
						EnableWindow(GetDlgItem(_hSelf, IDC_BTNWN2INSTALL), true);

						if (IsDlgButtonChecked(_hSelf, IDC_USENWN1))
							run_dlgProc(WM_COMMAND, IDC_USENWN1, 0);
						else
							run_dlgProc(WM_COMMAND, IDC_USENWN2, 0);
					}

					return FALSE;
				}

				case IDC_BTADDPATH:
				{
					TCHAR path[MAX_PATH] = { 0 };
					GetDlgItemText(_hSelf, IDC_TXTADDPATH, path, static_cast<int>(std::size(path)));
					generic_string newPath = path;
					if (!newPath.empty())
					{
						if (isValidDirectory(newPath.c_str()))
							ListBox_AddString(GetDlgItem(_hSelf, IDC_LSTADDPATH), newPath.c_str());
						else
						{
							MessageBox(_hSelf, (TEXT("Invalid or Inexistent Directory Name: ") + newPath).c_str(), TEXT("Parameter validation"), MB_OK | MB_ICONEXCLAMATION);
							return FALSE;
						}
					}

					SetDlgItemText(_hSelf, IDC_TXTADDPATH, TEXT(""));

					return TRUE;
				}

				case IDC_BTDELPATH:
				{
					int index = ListBox_GetCurSel(GetDlgItem(_hSelf, IDC_LSTADDPATH));
					if (index > -1)
						ListBox_DeleteString(GetDlgItem(_hSelf, IDC_LSTADDPATH), index);
					return FALSE;
				}

				case IDC_CHKOUTPUTDIR:
				{
					if (IsDlgButtonChecked(_hSelf, IDC_CHKOUTPUTDIR))
					{
						EnableWindow(GetDlgItem(_hSelf, IDC_TXTOUTPUTDIR), false);
						EnableWindow(GetDlgItem(_hSelf, IDC_BTOUTPUTDIR), false);
					}
					else
					{
						EnableWindow(GetDlgItem(_hSelf, IDC_TXTOUTPUTDIR), true);
						EnableWindow(GetDlgItem(_hSelf, IDC_BTOUTPUTDIR), true);
					}
					return FALSE;
				}

				case IDC_BTNWN1INSTALL:
				case IDC_BTNWN2INSTALL:
				case IDC_BTSEARCHPATH:
				case IDC_BTOUTPUTDIR:
				{
					TCHAR tempBuffer[MAX_PATH] = {};
					switch (wParam)
					{
					case IDC_BTNWN1INSTALL:
						GetDlgItemText(_hSelf, IDC_TXTNWN1INSTALL, tempBuffer, static_cast<int>(std::size(tempBuffer)));
						break;
					case IDC_BTNWN2INSTALL:
						GetDlgItemText(_hSelf, IDC_TXTNWN2INSTALL, tempBuffer, static_cast<int>(std::size(tempBuffer)));
						break;
					case IDC_BTSEARCHPATH:
						GetDlgItemText(_hSelf, IDC_TXTADDPATH, tempBuffer, static_cast<int>(std::size(tempBuffer)));
						break;
					case IDC_BTOUTPUTDIR:
						GetDlgItemText(_hSelf, IDC_TXTOUTPUTDIR, tempBuffer, static_cast<int>(std::size(tempBuffer)));
						break;
					}

					generic_string newPath;
					if (openFolderDialog(_hSelf, newPath, generic_string(tempBuffer)))
					{
						switch (wParam)
						{
						case IDC_BTNWN1INSTALL:
							SetDlgItemText(_hSelf, IDC_TXTNWN1INSTALL, newPath.c_str());
							break;
						case IDC_BTNWN2INSTALL:
							SetDlgItemText(_hSelf, IDC_TXTNWN2INSTALL, newPath.c_str());
							break;
						case IDC_BTSEARCHPATH:
							SetDlgItemText(_hSelf, IDC_TXTADDPATH, newPath.c_str());
							break;
						case IDC_BTOUTPUTDIR:
							SetDlgItemText(_hSelf, IDC_TXTOUTPUTDIR, newPath.c_str());
							break;
						}
					}
					return FALSE;
				}
			}
			break;
		}
	}

	// Signals done processing messages
	return FALSE;
}

INT_PTR CompilerSettingsDialog::doDialog()
{
	return ShowModal(IDD_COMPILERSETTINGS);
}

bool CompilerSettingsDialog::keepSettings()
{
	// Validation phase
	TCHAR tempBuffer[MAX_PATH] = {};
	bool bValid = true;
	generic_string errorString;
	errorString = TEXT("Invalid or Inexistent Directory Name: \"");
	if (!IsDlgButtonChecked(_hSelf, IDC_CHKIGNOREINSTALLPATHS))
	{
		if (IsDlgButtonChecked(_hSelf, IDC_USENWN1))
		{
			GetDlgItemText(_hSelf, IDC_TXTNWN1INSTALL, tempBuffer, static_cast<int>(std::size(tempBuffer)));
			if (!isValidDirectory(tempBuffer))
			{
				errorString.append(tempBuffer).append(TEXT("\""));
				MessageBox(_hSelf, errorString.c_str(), TEXT("Parameter validation"), MB_OK | MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(_hSelf, IDC_TXTNWN1INSTALL));
				bValid = false;
			}
		}
		else
		{
			GetDlgItemText(_hSelf, IDC_TXTNWN2INSTALL, tempBuffer, static_cast<int>(std::size(tempBuffer)));
			if (!isValidDirectory(tempBuffer))
			{
				errorString.append(tempBuffer).append(TEXT("\""));
				MessageBox(_hSelf, errorString.c_str(), TEXT("Parameter validation"), MB_OK | MB_ICONEXCLAMATION);
				SetFocus(GetDlgItem(_hSelf, IDC_TXTNWN2INSTALL));
				bValid = false;
			}
		}
	}

	if (!IsDlgButtonChecked(_hSelf, IDC_CHKOUTPUTDIR))
	{
		GetDlgItemText(_hSelf, IDC_TXTOUTPUTDIR, tempBuffer, static_cast<int>(std::size(tempBuffer)));
		if (!isValidDirectory(tempBuffer))
		{
			errorString.append(tempBuffer).append(TEXT("\""));
			MessageBox(_hSelf, errorString.c_str(), TEXT("Parameter validation"), MB_OK | MB_ICONEXCLAMATION);
			SetFocus(GetDlgItem(_hSelf, IDC_TXTOUTPUTDIR));
			bValid = false;
		}
	}

	if (!bValid)
		return false;

	// Saving phase
	Settings& myset = *_settings;

	myset.neverwinterInstallChoice = (IsDlgButtonChecked(_hSelf, IDC_USENWN1) ? 0 : 1);

	GetDlgItemText(_hSelf, IDC_TXTNWN1INSTALL, tempBuffer, static_cast<int>(std::size(tempBuffer)));
	myset.neverwinterOneInstallDir = properDirNameW(tempBuffer);
	GetDlgItemText(_hSelf, IDC_TXTNWN2INSTALL, tempBuffer, static_cast<int>(std::size(tempBuffer)));
	myset.neverwinterTwoInstallDir = properDirNameW(tempBuffer);

	myset.ignoreInstallPaths = IsDlgButtonChecked(_hSelf, IDC_CHKIGNOREINSTALLPATHS);

	// If user has unsaved input in additional folders, save for him.
	GetDlgItemText(_hSelf, IDC_TXTADDPATH, tempBuffer, static_cast<int>(std::size(tempBuffer)));
	if (tempBuffer[0] > 0)
	{
		if (static_cast<bool>(run_dlgProc(WM_COMMAND, IDC_BTADDPATH, 0)) == false)
			return false;
	}

	std::vector<generic_string> vData;
	for (int i = 0; i < ListBox_GetCount(GetDlgItem(_hSelf, IDC_LSTADDPATH)); i++)
	{
		generic_string strAdd;
		ListBox_GetText(GetDlgItem(_hSelf, IDC_LSTADDPATH), i, tempBuffer);
		strAdd = properDirNameW(tempBuffer);
		strAdd.append(TEXT(";"));
		vData.push_back(strAdd);
	}
	myset.setIncludeDirs(vData);

	myset.optimizeScript = IsDlgButtonChecked(_hSelf, IDC_CHKCOMPOPTIMIZE);
	myset.useNonBiowareExtenstions = IsDlgButtonChecked(_hSelf, IDC_CHKNONBIOWAREXTENSIONS);
	myset.generateSymbols = IsDlgButtonChecked(_hSelf, IDC_CHKCOMPNDBSYMBOLS);

	myset.compilerFlags = 0;
	myset.compilerFlags |= IsDlgButtonChecked(_hSelf, IDC_CHKCOMPSTRICTMODE) ? NscCompilerFlag_StrictModeEnabled : 0;
	myset.compilerFlags |= IsDlgButtonChecked(_hSelf, IDC_CHKCOMPMAKEFILE) ? NscCompilerFlag_GenerateMakeDeps : 0;
	myset.compilerFlags |= IsDlgButtonChecked(_hSelf, IDC_CHKCOMPDISABLESLASHPARSE) ? NscCompilerFlag_DisableDoubleQuote : 0;

	myset.compileVersion = (ComboBox_GetCurSel(GetDlgItem(_hSelf, IDC_CBOTARGETVERSION)) == 0) ? 174 : 169;

	myset.useScriptPathToCompile = IsDlgButtonChecked(_hSelf, IDC_CHKOUTPUTDIR);
	GetDlgItemText(_hSelf, IDC_TXTOUTPUTDIR, tempBuffer, static_cast<int>(std::size(tempBuffer)));
	myset.outputCompileDir = properDirNameW(tempBuffer);

	myset.compilerSettingsCreated = true;

	return true;
}
