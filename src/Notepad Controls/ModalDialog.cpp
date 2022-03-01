/** @file ModalDialog.cpp
 * Tweaks for StaticDialog to show a Modal Dialog instead.
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include <stdio.h>

#include "ModalDialog.h"

HGLOBAL ModalDialog::makeRTLResource(HINSTANCE hInst, int dialogID, DLGTEMPLATE** ppMyDlgTemplate)
{
	// Get Dlg Template resource
	HRSRC  hDialogRC = ::FindResource(hInst, MAKEINTRESOURCE(dialogID), RT_DIALOG);
	if (!hDialogRC)
		return NULL;

	HGLOBAL  hDlgTemplate = ::LoadResource(hInst, hDialogRC);
	if (!hDlgTemplate)
		return NULL;

	DLGTEMPLATE* pDlgTemplate = reinterpret_cast<DLGTEMPLATE*>(::LockResource(hDlgTemplate));
	if (!pDlgTemplate)
		return NULL;

	// Duplicate Dlg Template resource
	unsigned long sizeDlg = ::SizeofResource(hInst, hDialogRC);
	HGLOBAL hMyDlgTemplate = ::GlobalAlloc(GPTR, sizeDlg);
	*ppMyDlgTemplate = reinterpret_cast<DLGTEMPLATE*>(::GlobalLock(hMyDlgTemplate));

	::memcpy(*ppMyDlgTemplate, pDlgTemplate, sizeDlg);

	DLGTEMPLATEEX* pMyDlgTemplateEx = reinterpret_cast<DLGTEMPLATEEX*>(*ppMyDlgTemplate);
	if (pMyDlgTemplateEx->signature == 0xFFFF)
		pMyDlgTemplateEx->exStyle |= WS_EX_LAYOUTRTL;
	else
		(*ppMyDlgTemplate)->dwExtendedStyle |= WS_EX_LAYOUTRTL;

	return hMyDlgTemplate;
}

INT_PTR ModalDialog::ShowModal(HINSTANCE hInst, HWND hParent, int dialogID, bool isRTL)
{
	INT_PTR rValue;
	_hParent = hParent;

	if (isRTL)
	{
		DLGTEMPLATE* pMyDlgTemplate = NULL;
		HGLOBAL hMyDlgTemplate = makeRTLResource(hInst, dialogID, &pMyDlgTemplate);
		rValue = ::DialogBoxIndirectParam(hInst, pMyDlgTemplate, hParent, dlgProc, reinterpret_cast<LPARAM>(this));
		::GlobalFree(hMyDlgTemplate);
	}
	else
		rValue = ::DialogBoxParam(hInst, MAKEINTRESOURCE(dialogID), hParent, dlgProc, reinterpret_cast<LPARAM>(this));

	if (rValue < 1)
	{
		DWORD err = ::GetLastError();
		char errMsg[256];
		sprintf(errMsg, "DialogBoxParam() failed to create window.\rGetLastError() == %u", err);
		::MessageBoxA(NULL, errMsg, "In ModalDialog::ShowModal()", MB_OK | MB_ICONERROR);
		return rValue;
	}

	return rValue;
}

INT_PTR CALLBACK ModalDialog::dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		ModalDialog* pModalDlg = reinterpret_cast<ModalDialog*>(lParam);
		pModalDlg->_hSelf = hwnd;
		::GetWindowRect(hwnd, &(pModalDlg->_rc));
		pModalDlg->goToCenter();
		::SetWindowLongPtr(hwnd, GWLP_USERDATA, static_cast<LONG_PTR>(lParam));
		pModalDlg->run_dlgProc(message, wParam, lParam);

		return TRUE;
	}

	default:
	{
		ModalDialog* pModalDlg = reinterpret_cast<ModalDialog*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (!pModalDlg)
			return FALSE;
		return pModalDlg->run_dlgProc(message, wParam, lParam);
	}
	}
}


void ModalDialog::goToCenter()
{
	RECT rc;
	::GetClientRect(_hParent, &rc);
	POINT center = {};
	center.x = rc.left + (rc.right - rc.left) / 2;
	center.y = rc.top + (rc.bottom - rc.top) / 2;
	::ClientToScreen(_hParent, &center);

	int x = center.x - (_rc.right - _rc.left) / 2;
	int y = center.y - (_rc.bottom - _rc.top) / 2;

	::MoveWindow(_hSelf, x, y, _rc.right - _rc.left, _rc.bottom - _rc.top, true);
}

