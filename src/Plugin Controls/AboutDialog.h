/** @file AboutDialog.h
 * About Dialog Box
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include <map>
#include "StaticDialog.h"

class AboutDialog : public StaticDialog
{
public:
	AboutDialog() = default;

	void setReplaceStrings(const std::map<generic_string, generic_string>& replaceStrings) {
		_replaceStrings = replaceStrings;
	};
	void doDialog();

protected:
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	std::map<generic_string, generic_string> _replaceStrings;
};