/** @file AboutDialog.h
 * About Dialog Box
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once


#include <map>
#include "StaticDialog.h"
#include "AnchorMap.h"

#include "Common.h"
#include "OleCallback.h"



namespace NWScriptPlugin {

	class AboutDialog : public StaticDialog
	{
	public:
		AboutDialog() = default;

		void setReplaceStrings(const std::map<generic_string, generic_string>& replaceStrings) {
			_replaceStrings = replaceStrings;
		};

		void setHomePath(const TCHAR* homePath) {
			_homePath = homePath;			
		}

		void doDialog();

		void reloadAboutDocument() {
			if (_currentDocumentID == 0)
				return;
			LoadAboutTextEditor(_currentDocumentID);
		}

		void refreshDarkMode();

	protected:

		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:

		DECLARE_ANCHOR_MAP()

		void LoadAboutTextEditor(int resourceID);
		void LaunchHyperlink(const ENLINK& link);
		void setLogo();

		int _currentDocumentID = 0;

		generic_string _homePath;
		generic_string _aboutText;
		OleCallback* _aboutOleCallback = nullptr;
		std::map<generic_string, generic_string> _replaceStrings;

		// Anchoring and size restriction informations
		RECTSIZER mainWindowSize = {};
		RECTSIZER lblPluginName = {};
		RECTSIZER lblVersion = {};
		RECTSIZER lblCopyright = {};
		RECTSIZER lblCredits = {};
		RECTSIZER lnkHomepageSize = {};
		RECTSIZER txtAboutSize = {};
	};
}