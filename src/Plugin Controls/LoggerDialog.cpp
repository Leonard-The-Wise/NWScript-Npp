/** @file LoggerDialog.cpp
 * Logger Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include "pch.h"

#include "LoggerDialog.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK LoggerDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
		{
			_mainTabHwnd = GetDlgItem(_hSelf, IDC_TABLOGGER);

			// Create tabs
			TCITEM tie = { 0 };
			tie.mask = TCIF_TEXT | TCIF_IMAGE;
			tie.iImage = -1;
			tie.pszText = (TCHAR*)TEXT("Errors List");
			TabCtrl_InsertItem(_mainTabHwnd, 0, &tie);
			tie.pszText = (TCHAR*)TEXT("Console Output");
			TabCtrl_InsertItem(_mainTabHwnd, 1, &tie);



		}
		
		case WM_SIZE:
		{
			// Resize main tab
			RECT  rect;
			GetClientRect(_hSelf, &rect);
			SetWindowPos(_mainTabHwnd, NULL, 5, 5, rect.right-10, rect.bottom-10, SWP_NOZORDER);
		}

		case WM_COMMAND:
		{

		}

		default:
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}

	return FALSE;
}

