/** @file AboutDialog.h
 * About Dialog Box
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once
#include "StaticDialog.h"

class AboutDialog : public StaticDialog
{
public:
	AboutDialog() = default;

	void setConfigFile(const generic_string& s) {
		_notepadConfig = s;
	};
	void doDialog();

	virtual void destroy() {};

protected:
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
private:
	generic_string _notepadConfig;
};