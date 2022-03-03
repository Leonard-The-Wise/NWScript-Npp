/** @file FileAccessDialog.h
 * About Dialog Box
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once
#include <algorithm>
#include <vector>

#include "ModalDialog.h"
#include "Common.h"

class FileAccessDialog : public ModalDialog
{
public:
	FileAccessDialog() = default;

	// Sets the warning text display
	void SetWarning(const generic_string& sWarning)	{
		_sWarning = sWarning;
	}

	// Sets the text to display on the required files
	void SetFilesText(const std::vector<generic_string>& sFiles) {
		//create a copy since we are not modifying inputs
		std::vector<generic_string> sSorted = sFiles;
		std::sort(sSorted.begin(), sSorted.end());
		for (generic_string s : sSorted)
			_sFiles << s << TEXT("\r\n");
	}

	// Sets the solution string display
	void SetSolution(const generic_string& sSolution) {
		_sSolution = sSolution;
	}

	void SetAdminMode(bool bShowAdmin) {
		_bAdminMode = bShowAdmin;
	}

	void SetIcon(SHSTOCKICONID iconID) {
		_iconID = iconID;
	}

	// Displays the Dialog box
	virtual INT_PTR doDialog();

protected:
	virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	bool _bAdminMode = true;
	SHSTOCKICONID _iconID = SHSTOCKICONID::SIID_SHIELD;
	generic_string _sWarning = {};
	generic_string _sSolution = {};
	generic_stringstream _sFiles = {};
};