//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//::///////////////////////////////////////////////////////////////
//::
//:: Patched by Leonardo Silva (Feb-2022)
//:: - Turned some functions virtual to allow more overriding 
//:: - Changed some method's visibility as they are not supposed to
//::   be called directly.
//::
//::///////////////////////////////////////////////////////////////

#pragma once

#include "Window.h"
#include "Notepad_plus_msgs.h"

class StaticDialog : public Window
{
public :
	StaticDialog() : Window() { _rc = {}; };
	~StaticDialog(){
		if (isCreated()) {
			::SetWindowLongPtr(_hSelf, GWLP_USERDATA, (long)NULL);	//Prevent run_dlgProc from doing anything, since its virtual
			destroy();
		}
	};
	virtual void create(int dialogID, bool isRTL = false);

    virtual bool isCreated() const {
		return (_hSelf != NULL);
	};

    virtual void destroy() {
		::SendMessage(_hParent, NPPM_MODELESSDIALOG, MODELESSDIALOGREMOVE, (WPARAM)_hSelf);
		::DestroyWindow(_hSelf);
		_hSelf = nullptr;
	};

protected :
	RECT _rc;
	static INT_PTR CALLBACK dlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam) = 0;

	virtual void goToCenter();
	void alignWith(HWND handle, HWND handle2Align, PosAlign pos, POINT & point);
	HGLOBAL makeRTLResource(int dialogID, DLGTEMPLATE **ppMyDlgTemplate);
};

