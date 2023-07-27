/** @file ProcessFilesDialog.h
 * Process Files dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once
#include "ModalDialog.h"
#include "AnchorMap.h"

#include "Common.h"

namespace NWScriptPlugin {

	class WhatIsThisDialog : public ModalDialog
	{
	public:
		WhatIsThisDialog() = default;

		~WhatIsThisDialog() {
			if (_hWindowIcon)
				DeleteObject(_hWindowIcon);
		}

		INT_PTR doDialog();

		void setInterruptFlag(std::atomic<bool>& interruptFlagVariable) {
			_interruptFlagVariable = &interruptFlagVariable;
		}

		void lockWindow(bool toLock) {
			EnableWindow(_hSelf, !toLock);
		}

	protected:
		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
		std::atomic<bool>* _interruptFlagVariable;

	private:

		DECLARE_ANCHOR_MAP()

		// Anchoring and size restriction informations
		RECTSIZER mainWindowSize = {};
		RECTSIZER txtHelpSize = {};

		generic_string _helpText;
		int _currentDocumentID = 0;

		HICON _hWindowIcon = nullptr;

		void LoadHelpTextEditor(int resourceID);
		void LaunchHyperlink(const ENLINK& link);
	};
}
