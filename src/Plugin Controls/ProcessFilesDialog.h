/** @file ProcessFilesDialog.h
 * Process Files dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once
#include "StaticDialog.h"

namespace NWScriptPlugin {

	class ProcessFilesDialog : public StaticDialog
	{
	public:
		ProcessFilesDialog() = default;

		void doDialog();

		virtual void destroy() {};

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	};

}