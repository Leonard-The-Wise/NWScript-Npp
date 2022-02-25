/** @file Warning.h
 * Warning Dialog Box
 *
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once
#include "StaticDialog.h"

class WarningDialog : public StaticDialog
{
public:
	WarningDialog() = default;

	void doDialog();

	virtual void destroy() {};

protected:
	virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
};