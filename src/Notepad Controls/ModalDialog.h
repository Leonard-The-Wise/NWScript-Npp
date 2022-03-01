/** @file ModalDialog.h
 * A Modal Dialog box template
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "Window.h"

class ModalDialog 
{
public:
	ModalDialog() : _hSelf(nullptr), _rc(), _hParent(nullptr)
	{ };
	~ModalDialog() {
		if (_hSelf) {
			::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (long)NULL);	//Prevent run_dlgProc from doing anything, since its virtual
			::DestroyWindow(_hSelf);
		}
	};

	// Derived classes should implement this
	virtual INT_PTR doDialog(HINSTANCE hInst, HWND hParent) = 0;

protected:
	// Derived classes should implement this
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

	// Internal functions
	HWND _hSelf;
	virtual INT_PTR ShowModal(HINSTANCE hInst, HWND hParent, int dialogID, bool isRTL = false);
	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	HGLOBAL makeRTLResource(HINSTANCE hInst, int dialogID, DLGTEMPLATE** ppMyDlgTemplate);
private:
	HWND _hParent;
	RECT _rc;
	void goToCenter();
};

