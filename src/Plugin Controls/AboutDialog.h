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

	protected:

		virtual intptr_t CALLBACK run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam);
	private:

		DECLARE_ANCHOR_MAP()

		void LoadAboutTextEditor();
		void LaunchHyperlink(const ENLINK& link);

		generic_string _homePath;
		generic_string _aboutText;
		OleCallback _oleCallback;
		std::map<generic_string, generic_string> _replaceStrings;
	};
}