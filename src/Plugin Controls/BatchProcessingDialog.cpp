/** @file BatchProcessing.cpp
 * Batch file processing dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include <sstream>
//#include <Shlwapi.h>

#include "Settings.h"
#include "BatchProcessingDialog.h"

#include "PluginControlsRC.h"
#include "PluginDarkMode.h"

typedef jpcre2::select<TCHAR> pcre2;

using namespace NWScriptPlugin;

intptr_t CALLBACK BatchProcessingDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			_dpiManager.resizeControl(_hSelf);
			_dpiManager.resizeChildren(_hSelf, true);

			PluginDarkMode::autoSetupWindowAndChildren(_hSelf);

			Settings& myset = *_settings;

			SetDlgItemText(_hSelf, IDC_TXTBATCHDIRSTART, myset.startingBatchFolder.c_str());

			_tmpFiltersCompile = myset.getFileFiltersCompile();
			_tmpFiltersDisasm = myset.getFileFiltersDisasm();

			if (myset.batchCompileMode == 0)
			{
				::CheckRadioButton(_hSelf, IDC_RDCOMPILE, IDC_RDDISASM, IDC_RDCOMPILE);
				SetDlgItemText(_hSelf, IDC_TXTBATCHFILTERS, _tmpFiltersCompile.c_str());
			}
			else
			{
				::CheckRadioButton(_hSelf, IDC_RDCOMPILE, IDC_RDDISASM, IDC_RDDISASM);
				SetDlgItemText(_hSelf, IDC_TXTBATCHFILTERS, _tmpFiltersDisasm.c_str());
			}

			CheckDlgButton(_hSelf, IDC_CHKRECURSIVE, myset.recurseSubFolders ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(_hSelf, IDC_CHKCONTINUEONFAIL, myset.continueCompileOnFail ? BST_CHECKED : BST_UNCHECKED);

			SetDlgItemText(_hSelf, IDC_TXTOUTPUTDIRBATCH, myset.batchOutputCompileDir.c_str());
			CheckDlgButton(_hSelf, IDC_CHKOUTPUTDIRBATCH, myset.useScriptPathToBatchCompile);
			if (myset.useScriptPathToBatchCompile)
			{
				EnableWindow(GetDlgItem(_hSelf, IDC_TXTOUTPUTDIRBATCH), false);
				EnableWindow(GetDlgItem(_hSelf, IDC_BTOUTPUTDIRBATCH), false);
			}

			// Window icon
			_hWindowIcon = loadSVGFromResourceIcon(_hInst, IDI_COMPILEBATCH, PluginDarkMode::isEnabled(), _dpiManager.scaleX(16), _dpiManager.scaleY(16));
			::SendMessage(_hSelf, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(_hWindowIcon));

			return TRUE;
		}

		case WM_COMMAND:
		{
			switch (wParam)
			{
				case IDC_CHKOUTPUTDIRBATCH:
				{
					if (IsDlgButtonChecked(_hSelf, IDC_CHKOUTPUTDIRBATCH))
					{
						EnableWindow(GetDlgItem(_hSelf, IDC_TXTOUTPUTDIRBATCH), false);
						EnableWindow(GetDlgItem(_hSelf, IDC_BTOUTPUTDIRBATCH), false);
					}
					else
					{
						EnableWindow(GetDlgItem(_hSelf, IDC_TXTOUTPUTDIRBATCH), true);
						EnableWindow(GetDlgItem(_hSelf, IDC_BTOUTPUTDIRBATCH), true);
					}
					return FALSE;
				}

				case IDC_RDCOMPILE:
				{
					SetDlgItemText(_hSelf, IDC_TXTBATCHFILTERS, _tmpFiltersCompile.c_str());
					return FALSE;
				}

				case IDC_RDDISASM:
				{
					SetDlgItemText(_hSelf, IDC_TXTBATCHFILTERS, _tmpFiltersDisasm.c_str());
					return FALSE;
				}

				case IDC_BTBATCHDIRSTART:
				case IDC_BTOUTPUTDIRBATCH:
				{
					TCHAR tempBuffer[MAX_PATH] = {};
					switch (wParam)
					{
					case IDC_BTBATCHDIRSTART:
						GetDlgItemText(_hSelf, IDC_TXTBATCHDIRSTART, tempBuffer, std::size(tempBuffer));
						break;
					case IDC_BTOUTPUTDIRBATCH:
						GetDlgItemText(_hSelf, IDC_TXTOUTPUTDIRBATCH, tempBuffer, std::size(tempBuffer));
						break;
					}
					generic_string newPath;
					if (openFolderDialog(_hSelf, newPath, generic_string(tempBuffer)))
					{
						switch (wParam)
						{
						case IDC_BTBATCHDIRSTART:
							SetDlgItemText(_hSelf, IDC_TXTBATCHDIRSTART, newPath.c_str());
							break;
						case IDC_BTOUTPUTDIRBATCH:
							SetDlgItemText(_hSelf, IDC_TXTOUTPUTDIRBATCH, newPath.c_str());
							break;
						}
					}
					return FALSE;
				}

				case IDCANCEL:
				case IDCLOSE:
				case IDSTART:
					if (!keepSettings())
						return FALSE;
					display(false);
					destroy();
					if (_okDialogCallback != nullptr)
						_okDialogCallback(static_cast<HRESULT>(wParam));
					return TRUE;
			}

			if (HIWORD(wParam) == EN_UPDATE)
			{
				if (LOWORD(wParam) == IDC_TXTBATCHFILTERS)
				{
					TCHAR buffer[1024];
					GetDlgItemText(_hSelf, IDC_TXTBATCHFILTERS, buffer, std::size(buffer));
					if (IsDlgButtonChecked(_hSelf, IDC_RDCOMPILE))
						_tmpFiltersCompile = buffer;
					else
						_tmpFiltersDisasm = buffer;
				}
			}
			break;
		}
	}

	// Signals done processing messages
	return FALSE;
}

void BatchProcessingDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_BATCHPROCESS);

	//Show and centralize
	goToCenter();
}


bool BatchProcessingDialog::keepSettings()
{
	// Validation phase
	TCHAR tempBuffer[MAX_PATH] = {};
	generic_string testString;
	bool bValid = true;
	generic_string errorString;
	errorString = TEXT("Invalid or Inexistent Directory: \"");

	GetDlgItemText(_hSelf, IDC_TXTBATCHDIRSTART, tempBuffer, std::size(tempBuffer));
	testString = tempBuffer;
	if (!testString.empty())
	{
		if (!PathFileExists(testString.c_str()))
		{
			errorString.append(testString).append(TEXT("\""));
			MessageBox(_hSelf, errorString.c_str(), TEXT("Parameter validation"), MB_OK | MB_ICONEXCLAMATION);
			SetFocus(GetDlgItem(_hSelf, IDC_TXTBATCHDIRSTART));
			bValid = false;
		}
	}

	if (!IsDlgButtonChecked(_hSelf, IDC_CHKOUTPUTDIRBATCH))
	{
		GetDlgItemText(_hSelf, IDC_TXTOUTPUTDIRBATCH, tempBuffer, std::size(tempBuffer));
		if (!PathFileExists(tempBuffer))
		{
			errorString.append(tempBuffer).append(TEXT("\""));
			MessageBox(_hSelf, errorString.c_str(), TEXT("Parameter validation"), MB_OK | MB_ICONEXCLAMATION);
			SetFocus(GetDlgItem(_hSelf, IDC_TXTOUTPUTDIRBATCH));
			bValid = false;
		}
	}

	if (!bValid)
		return false;

	// Saving phase
	Settings& myset = *_settings;

	GetDlgItemText(_hSelf, IDC_TXTBATCHDIRSTART, tempBuffer, std::size(tempBuffer));
	myset.startingBatchFolder = properDirNameW(tempBuffer);

	myset.setFileFiltersCompile(_tmpFiltersCompile);
	myset.setFileFiltersDisasm(_tmpFiltersDisasm);

	myset.recurseSubFolders = IsDlgButtonChecked(_hSelf, IDC_CHKRECURSIVE);
	myset.continueCompileOnFail = IsDlgButtonChecked(_hSelf, IDC_CHKCONTINUEONFAIL);

	myset.batchCompileMode = IsDlgButtonChecked(_hSelf, IDC_RDCOMPILE) ? 0 : 1;

	myset.useScriptPathToBatchCompile = IsDlgButtonChecked(_hSelf, IDC_CHKOUTPUTDIRBATCH);
	GetDlgItemText(_hSelf, IDC_TXTOUTPUTDIRBATCH, tempBuffer, std::size(tempBuffer));
	myset.batchOutputCompileDir = properDirNameW(tempBuffer);

	return true;
}

