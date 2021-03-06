/** @file PathAccessDialog.h
 * About Dialog Box
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once
//#include <algorithm>
//#include <vector>
//#include <sstream>

#include "ModalDialog.h"
#include "Common.h"

namespace NWScriptPlugin {

	class PathAccessDialog : public ModalDialog
	{
	public:
		PathAccessDialog() = default;
		
		~PathAccessDialog() {
			DeleteObject(_hShield);
			DeleteObject(_hShieldSmall);
			DeleteObject(_hWindowIcon);
		}

		// Sets the warning text display
		void SetWarning(const generic_string& sWarning) {
			_sWarning = sWarning;
		}

		// Sets the text to display on the required files
		void SetPathsText(const std::vector<generic_string>& sPaths) {
			//create a copy since we are not modifying inputs
			std::vector<generic_string> sSorted = sPaths;
			std::sort(sSorted.begin(), sSorted.end());
			for (generic_string s : sSorted)
				_sPaths << s << TEXT("\r\n");
		}

		// Sets the solution string display
		void SetSolution(const generic_string& sSolution) {
			_sSolution = sSolution;
		}

		void SetAdminMode(bool bShowAdmin) {
			_bAdminMode = bShowAdmin;
		}

		void SetIcon(SHSTOCKICONID iconID) {
			_iconID = iconID;
		}

		void SetMorphToCopyMode() {
			_morphToCopy = true;
		}

		// Displays the Dialog box
		virtual INT_PTR doDialog();

	protected:
		virtual INT_PTR CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:
		bool _bAdminMode = true;
		bool _morphToCopy = false;
		SHSTOCKICONID _iconID = SHSTOCKICONID::SIID_SHIELD;
		generic_string _sWarning = {};
		generic_string _sSolution = {};
		generic_stringstream _sPaths = {};

		HBITMAP _hShield;
		HICON _hShieldSmall;
		HICON _hWindowIcon;

		void MorphToPluginCopyMode();
	};

}
