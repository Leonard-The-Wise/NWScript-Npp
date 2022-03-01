/** @file ElevateDialog.h
 * About Dialog Box
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <vector>
#include "ModalDialog.h"
#include "Common.h"

class ElevateDialog : public ModalDialog
{
public:
	ElevateDialog() = default;

	// Sets the text to display on the required files
	void SetFilesText(const std::vector<generic_string>& sFiles) {
		for (generic_string s : sFiles)
			_sFiles << s << TEXT("\r\n");
	}

	// Displays the Dialog box
	virtual INT_PTR doDialog(HINSTANCE hInst, HWND hParent);

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	generic_stringstream _sFiles = {};
};