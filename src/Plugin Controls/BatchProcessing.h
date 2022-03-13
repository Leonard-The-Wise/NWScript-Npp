/** @file BatchProcessing.h
 * Batch file processing dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "StaticDialog.h"

namespace NWScriptPlugin {

	class BatchProcessingDialog : public StaticDialog
	{
	public:
		BatchProcessingDialog() = default;

		void doDialog();

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
	};

}