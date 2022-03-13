/** @file CompilerSettings.h
 * Compiler settings dialog box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#include "StaticDialog.h"
#include "CompilerSettings.h"

namespace NWScriptPlugin {

	class CompilerSettingsDialog : public StaticDialog
	{
	public:
		CompilerSettingsDialog() = default;

		void doDialog();

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
	};

}