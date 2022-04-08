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

		void setInterruptFlag(std::atomic<bool>& interruptFlagVariable) {
			_interruptFlagVariable = &interruptFlagVariable;
		}

		void setStatus(const generic_string& status) {
			SetDlgItemText(_hSelf, IDC_LBLSTATUS, status.c_str());
		}

		void lockWindow(bool toLock) {
			EnableWindow(_hSelf, !toLock);
		}


	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
		std::atomic<bool>* _interruptFlagVariable;
	};

}