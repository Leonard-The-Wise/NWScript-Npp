/** @file BatchProcessing.cpp
 * Batch file processing dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include <Windows.h>
#include <Commctrl.h>
#include <sstream>

#include "PluginMain.h"
#include "BatchProcessing.h"

#include "PluginControlsRC.h"


using namespace NWScriptPlugin;

intptr_t CALLBACK BatchProcessingDialog::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
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
		case IDCLOSE:
				// Check settings and fall back to IDOK
		case IDOK:
			display(false);
			destroy();
			return TRUE;
		}
	}
	}

	// Signals done processing messages
	return FALSE;
}

void BatchProcessingDialog::doDialog()
{
	// Create from resource
	if (!isCreated())
		create(IDD_BATCHPROCESS);

	//Show and centralize
	goToCenter();
}