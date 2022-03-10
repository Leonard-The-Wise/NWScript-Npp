/** @file ModalDialog.h
 * A Modal Dialog box template
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "Window.h"

class ModalDialog 
{
public:
	ModalDialog() : _hSelf(nullptr), _rc(), _hParent(nullptr), _hInst(nullptr)
	{ };
	~ModalDialog() {
		if (_hSelf) {
			::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (long)NULL);	//Prevent run_dlgProc from doing anything, since its virtual
			::DestroyWindow(_hSelf);
		}
	};

	void init(HINSTANCE hInst, HWND hParent) {
		_hInst = hInst, _hParent = hParent;
	}

	bool isInitialized() {
		return (_hInst && _hParent);
	}

	// Derived classes should implement this
	virtual INT_PTR doDialog() = 0;

	virtual void destroy() {
		::DestroyWindow(_hSelf);
		_hSelf = nullptr;
	};

protected:
	// Derived classes should implement this
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

	// Internal functions
	HWND _hSelf;
	INT_PTR ShowModal(int dialogID, bool isRTL = false);
	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	HGLOBAL makeRTLResource(int dialogID, DLGTEMPLATE** ppMyDlgTemplate);

private:
	HINSTANCE _hInst;
	HWND _hParent;
	RECT _rc;
	void goToCenter();
};

