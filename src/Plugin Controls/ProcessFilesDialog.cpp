/** @file ProcessFilesDialog.cpp
 * Process Files dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <Windows.h>
//#include <Commctrl.h>
//#include <sstream>

#include "PluginMain.h"
#include "ProcessFilesDialog.h"

#include "PluginControlsRC.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK ProcessFilesDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
	{
		return TRUE;
	}

	case WM_COMMAND:
	{
		switch (wParam)
		{
		case IDCANCEL:
			display(false);
			destroy();
			return TRUE;
		}
	}
	}

	// Signals done processing messages
	return FALSE;
}

void ProcessFilesDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_PROCESSFILES);

	//Show and centralize
	goToCenter();
}