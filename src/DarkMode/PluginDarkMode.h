// This file is part of Notepad++ project
// Copyright (c) 2021 adzm / Adam D. Walling

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#pragma once

#define BKLUMINANCE_BRIGHTER 140
#define BKLUMINANCE_SOFTER 80

// Defined according to documentation:
// https://source.winehq.org/WineAPI/ColorRGBToHLS.html
#define HLSMAXRANGE 240

constexpr COLORREF HEXRGB(DWORD rrggbb) {
	// from 0xRRGGBB like natural #RRGGBB
	// to the little-endian 0xBBGGRR
	return
		((rrggbb & 0xFF0000) >> 16) |
		((rrggbb & 0x00FF00) ) |
		((rrggbb & 0x0000FF) << 16);
}

namespace PluginDarkMode
{
	struct Colors
	{
		COLORREF background = 0;
		COLORREF softerBackground = 0;
		COLORREF hotBackground = 0;
		COLORREF pureBackground = 0;
		COLORREF errorBackground = 0;
		COLORREF text = 0;
		COLORREF darkerText = 0;
		COLORREF disabledText = 0;
		COLORREF linkText = 0;
		COLORREF edge = 0;
	};

	struct Options
	{
		bool enable = false;
		bool enableMenubar = false;
	};

	enum class ToolTipsType
	{
		tooltip,
		toolbar,
		listview,
		treeview,
		tabbar
	};

	enum class ColorTone {
		blackTone  = 0,
		redTone    = 1,
		greenTone  = 2,
		blueTone   = 3,
		purpleTone = 4,
		cyanTone   = 5,
		oliveTone  = 6,
		customizedTone = 32
	};

	enum class TreeViewStyle
	{
		classic = 0,
		light = 1,
		dark = 2
	};

	struct HLS {
		WORD h = 0;
		WORD l = 0;
		WORD s = 0;
	};

	void initDarkMode();

	bool isInitialized();
	bool isEnabled();
	bool isDarkMenuEnabled();
	bool isExperimentalSupported();

	bool isWindows11();

	COLORREF invertLightness(COLORREF c);
	COLORREF invertLightnessSofter(COLORREF c);
	COLORREF lightColor(COLORREF color, WORD luminance);
	bool colorizeBitmap(HBITMAP image, WORD h = 0, WORD l = 0, WORD s = 0,
		bool changeH = false, bool changeL = false, bool changeS = false, bool testAlphaForChange = true,
		short rotateH = 0, short displaceL = 0, short displaceS = 0, bool testAlphaForDisplace = true,
		bool invertR = false, bool invertG = false, bool invertB = false, bool testAlphaForInvertColor = false,
		bool invertLight = false, WORD invertLightCeiling = 240, WORD invertLightFloor = 0, bool testAlphaForInvertLight = false,
		WORD luminosityMin = 0, WORD luminosityMax = 240, bool useLuminosityMin = false, bool useLuminosityMax = false, 
		bool testAlphaForLuminosityThreshold = false, bool alphaUnpremultiply = false);
	double calculatePerceivedLighness(COLORREF c);
	HBITMAP createCustomThemeBackgroundBitmap(HTHEME hTheme, int iPartID, int iStateID, WORD extraLuminance = 0);

	void setDarkTone(ColorTone colorToneChoice);

	COLORREF getBackgroundColor();
	COLORREF getSofterBackgroundColor();
	COLORREF getHotBackgroundColor();
	COLORREF getDarkerBackgroundColor();
	COLORREF getErrorBackgroundColor();

	COLORREF getTextColor();
	COLORREF getDarkerTextColor();
	COLORREF getDisabledTextColor();
	COLORREF getLinkTextColor();

	COLORREF getEdgeColor();

	HBRUSH getBackgroundBrush();
	HBRUSH getDarkerBackgroundBrush();
	HBRUSH getSofterBackgroundBrush();
	HBRUSH getHotBackgroundBrush();
	HBRUSH getErrorBackgroundBrush();
	HBRUSH getHardlightBackgroundBrush();
	HBRUSH getSoftlightBackgroundBrush();

	HPEN getDarkerTextPen();
	HPEN getEdgePen();

	void setThemeColors(Colors& newColors);
	void setBackgroundColor(COLORREF c);
	void setSofterBackgroundColor(COLORREF c);
	void setHotBackgroundColor(COLORREF c);
	void setDarkerBackgroundColor(COLORREF c);
	void setErrorBackgroundColor(COLORREF c);
	void setTextColor(COLORREF c);
	void setDarkerTextColor(COLORREF c);
	void setDisabledTextColor(COLORREF c);
	void setLinkTextColor(COLORREF c);
	void setEdgeColor(COLORREF c);

	Colors getDarkModeDefaultColors();
	void changeCustomTheme(const Colors& colors);

	// handle events
	void handleSettingChange(HWND hwnd, LPARAM lParam);

	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr);
	void drawUAHMenuNCBottomLine(HWND hWnd);

	// from DarkMode.h
	void initExperimentalDarkMode();
	void setDarkMode(bool useDark, bool fixDarkScrollbar);
	void allowDarkModeForApp(bool allow);
	bool allowDarkModeForWindow(HWND hWnd, bool allow);
	void setTitleBarThemeColor(HWND hWnd);

	// enhancements to DarkMode.h
	void enableDarkScrollBarForWindowAndChildren(HWND hwnd);

	void subclassButtonControl(HWND hwnd);
	void subclassComboBoxControl(HWND hwnd);
	void subclassGroupboxControl(HWND hwnd);
	void subclassListViewControl(HWND hwnd);
	void subclassTabControl(HWND hwnd);
	void subclassWindow(HWND hwnd);

	void autoSetupWindowAndChildren(HWND hwndWindow);
	void autoThemeChildControls(HWND hwndParent);
	void autoSubclassAndThemeChildControls(HWND hwndParent, bool subclass = true, bool theme = true);

	void setDarkTitleBar(HWND hwnd);
	void setDarkExplorerTheme(HWND hwnd);
	void setDarkScrollBar(HWND hwnd);
	void setDarkTooltips(HWND hwnd, ToolTipsType type);
	void setDarkLineAbovePanelToolbar(HWND hwnd);
	void setDarkListView(HWND hwnd);
	void calculateTreeViewStyle();
	void setTreeViewStyle(HWND hwnd);

	void disableVisualStyle(HWND hwnd, bool doDisable);
	void setBorder(HWND hwnd, bool border = true);

	BOOL CALLBACK enumAutocompleteProc(HWND hwnd, LPARAM lParam);
	void setDarkAutoCompletion();

	LRESULT onCtlColor(HDC hdc);
	LRESULT onCtlColorSofter(HDC hdc);
	LRESULT onCtlColorDarker(HDC hdc);
	LRESULT onCtlColorError(HDC hdc);
	LRESULT onCtlColorDarkerBGStaticText(HDC hdc, bool isTextEnabled);
}

