﻿// This file is part of Notepad++ project
// Copyright (C)2021 adzm / Adam D. Walling

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

// Which classes exist for themes?
// https://stackoverflow.com/questions/217532/what-are-the-possible-classes-for-the-openthemedata-function
//
// Controls styles and states info:
// https://docs.microsoft.com/en-us/windows/win32/controls/control-spy
//

// Generate debug info for this file
//#define _DEBUG_DARK_MODE

// Enable or disable precompiled headers
#define USE_PCH 1

#if (USE_PCH == 1)
#include "pch.h"
#else
#include <Windows.h>
#include <Uxtheme.h>
#include <Vssym32.h>
#include <Shlwapi.h>
#include <Richedit.h>
#include <cmath>
#include <assert.h>
#endif

#include "DPIManager.h"
#include "PluginDarkMode.h"

#include "DarkMode.h"
#include "UAHMenuBar.h"

#ifdef __GNUC__
#define WINAPI_LAMBDA WINAPI
#else
#define WINAPI_LAMBDA
#endif

#pragma comment(lib, "uxtheme.lib")

#define BKLUMINANCE_BRIGHTER 140
#define BKLUMINANCE_SOFTER 80

namespace PluginDarkMode
{
	//Globals
	ColorTone g_colorToneChoice = ColorTone::blackTone;
	static DPIManager _dpiManager;      // dpi manager for some functions
	static Options _options;			// actual runtime options

	TreeViewStyle treeViewStyle = TreeViewStyle::classic;
	COLORREF treeViewBg = 0;
	double lighnessTreeView = 50.0;

	struct Brushes
	{
		HBRUSH background = nullptr;
		HBRUSH softerBackground = nullptr;
		HBRUSH hotBackground = nullptr;
		HBRUSH pureBackground = nullptr;
		HBRUSH errorBackground = nullptr;
		HBRUSH hardlightBackground = nullptr;
		HBRUSH softlightBackground = nullptr;
		HBRUSH textColorBrush = nullptr;
		HBRUSH darkerTextColorBrush = nullptr;

		Brushes(const Colors& colors)
			: background(::CreateSolidBrush(colors.background))
			, softerBackground(::CreateSolidBrush(colors.softerBackground))
			, hotBackground(::CreateSolidBrush(colors.hotBackground))
			, pureBackground(::CreateSolidBrush(colors.pureBackground))
			, errorBackground(::CreateSolidBrush(colors.errorBackground))
			, hardlightBackground(::CreateSolidBrush(lightColor(colors.background, BKLUMINANCE_BRIGHTER)))
			, softlightBackground(::CreateSolidBrush(lightColor(colors.background, BKLUMINANCE_SOFTER)))
			, textColorBrush(::CreateSolidBrush(colors.text))
			, darkerTextColorBrush(::CreateSolidBrush(colors.darkerText))

		{}

		~Brushes()
		{
			::DeleteObject(background);			background = nullptr;
			::DeleteObject(softerBackground);	softerBackground = nullptr;
			::DeleteObject(hotBackground);		hotBackground = nullptr;
			::DeleteObject(pureBackground);		pureBackground = nullptr;
			::DeleteObject(errorBackground);	errorBackground = nullptr;
			::DeleteObject(hardlightBackground);	hardlightBackground = nullptr;
			::DeleteObject(softlightBackground);	softlightBackground = nullptr;
			::DeleteObject(textColorBrush);			textColorBrush = nullptr;
			::DeleteObject(darkerTextColorBrush);	darkerTextColorBrush = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(background);
			::DeleteObject(softerBackground);
			::DeleteObject(hotBackground);
			::DeleteObject(pureBackground);
			::DeleteObject(errorBackground);
			::DeleteObject(hardlightBackground);
			::DeleteObject(softlightBackground);
			::DeleteObject(textColorBrush);
			::DeleteObject(darkerTextColorBrush);

			background = ::CreateSolidBrush(colors.background);
			softerBackground = ::CreateSolidBrush(colors.softerBackground);
			hotBackground = ::CreateSolidBrush(colors.hotBackground);
			pureBackground = ::CreateSolidBrush(colors.pureBackground);
			errorBackground = ::CreateSolidBrush(colors.errorBackground);
			hardlightBackground = ::CreateSolidBrush(lightColor(colors.background, BKLUMINANCE_BRIGHTER));
			softlightBackground = ::CreateSolidBrush(lightColor(colors.background, BKLUMINANCE_SOFTER));
			textColorBrush = ::CreateSolidBrush(colors.text);
			darkerTextColorBrush = ::CreateSolidBrush(colors.darkerText);
		}
	};

	struct Pens
	{
		HPEN darkerTextPen = nullptr;
		HPEN edgePen = nullptr;
		HPEN lightEdgePen = nullptr;
		HPEN darkEdgePen = nullptr;

		Pens(const Colors& colors)
			: darkerTextPen(::CreatePen(PS_SOLID, 1, colors.darkerText))
			, edgePen(::CreatePen(PS_SOLID, 1, colors.edge))
			, lightEdgePen(::CreatePen(PS_SOLID, 1, lightColor(colors.edge, 220)))
			, darkEdgePen(::CreatePen(PS_SOLID, 1, lightColor(colors.edge, 60)))
		{}

		~Pens()
		{
			::DeleteObject(darkerTextPen);	darkerTextPen = nullptr;
			::DeleteObject(edgePen);		edgePen = nullptr;
			::DeleteObject(lightEdgePen);	lightEdgePen = nullptr;
			::DeleteObject(darkEdgePen);	darkEdgePen = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(darkerTextPen);
			::DeleteObject(edgePen);
			::DeleteObject(lightEdgePen);
			::DeleteObject(darkEdgePen);

			darkerTextPen = ::CreatePen(PS_SOLID, 1, colors.darkerText);
			edgePen = ::CreatePen(PS_SOLID, 1, colors.edge);
			lightEdgePen = ::CreatePen(PS_SOLID, 1, lightColor(colors.edge, 220));
			darkEdgePen = ::CreatePen(PS_SOLID, 1, lightColor(colors.edge, 60));
		}

	};

	// black (default)
	static const Colors darkColors{
		HEXRGB(0x202020),	// background
		HEXRGB(0x404040),	// softerBackground
		HEXRGB(0x404040),	// hotBackground
		HEXRGB(0x202020),	// pureBackground
		HEXRGB(0xB00000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFC000),	// linkTextColor
		HEXRGB(0x646464)	// edgeColor
	};

	// red tone
	static const Colors darkRedColors{
		HEXRGB(0x302020),	// background
		HEXRGB(0x504040),	// softerBackground
		HEXRGB(0x504040),	// hotBackground
		HEXRGB(0x302020),	// pureBackground
		HEXRGB(0xC00000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFC000),	// linkTextColor
		HEXRGB(0x908080)	// edgeColor
	};

	// green tone
	static const Colors darkGreenColors{
		HEXRGB(0x203020),	// background
		HEXRGB(0x405040),	// softerBackground
		HEXRGB(0x405040),	// hotBackground
		HEXRGB(0x203020),	// pureBackground
		HEXRGB(0xB01000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x809080)	// edgeColor
	};

	// blue tone
	static const Colors darkBlueColors{
		HEXRGB(0x202040),	// background
		HEXRGB(0x404060),	// softerBackground
		HEXRGB(0x404060),	// hotBackground
		HEXRGB(0x202040),	// pureBackground
		HEXRGB(0xB00020),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x8080A0)	// edgeColor
	};

	// purple tone
	static const Colors darkPurpleColors{
		HEXRGB(0x302040),	// background
		HEXRGB(0x504060),	// softerBackground
		HEXRGB(0x504060),	// hotBackground
		HEXRGB(0x302040),	// pureBackground
		HEXRGB(0xC00020),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x9080A0)	// edgeColor
	};

	// cyan tone
	static const Colors darkCyanColors{
		HEXRGB(0x203040),	// background
		HEXRGB(0x405060),	// softerBackground
		HEXRGB(0x405060),	// hotBackground
		HEXRGB(0x203040),	// pureBackground
		HEXRGB(0xB01020),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x8090A0)	// edgeColor
	};

	// olive tone
	static const Colors darkOliveColors{
		HEXRGB(0x303020),	// background
		HEXRGB(0x505040),	// softerBackground
		HEXRGB(0x505040),	// hotBackground
		HEXRGB(0x303020),	// pureBackground
		HEXRGB(0xC01000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x909080)	// edgeColor
	};

	// customized
	Colors darkCustomizedColors{
		HEXRGB(0x202020),	// background
		HEXRGB(0x404040),	// softerBackground
		HEXRGB(0x404040),	// hotBackground
		HEXRGB(0x202020),	// pureBackground
		HEXRGB(0xB00000),	// errorBackground
		HEXRGB(0xE0E0E0),	// textColor
		HEXRGB(0xC0C0C0),	// darkerTextColor
		HEXRGB(0x808080),	// disabledTextColor
		HEXRGB(0xFFFF00),	// linkTextColor
		HEXRGB(0x646464)	// edgeColor
	};

	void setDarkTone(ColorTone colorToneChoice)
	{
		g_colorToneChoice = colorToneChoice;
	}

	struct Theme
	{
		Colors _colors;
		Brushes _brushes;
		Pens _pens;

		Theme(const Colors& colors)
			: _colors(colors)
			, _brushes(colors)
			, _pens(colors)
		{}

		void change(const Colors& colors)
		{
			_colors = colors;
			_brushes.change(colors);
			_pens.change(colors);
		}
	};

	Theme tDefault(darkColors);
	Theme tR(darkRedColors);
	Theme tG(darkGreenColors);
	Theme tB(darkBlueColors);
	Theme tP(darkPurpleColors);
	Theme tC(darkCyanColors);
	Theme tO(darkOliveColors);

	Theme tCustom(darkCustomizedColors);

	Theme& getTheme()
	{
		switch (g_colorToneChoice)
		{
			case ColorTone::redTone:
				return tR;

			case ColorTone::greenTone:
				return tG;

			case ColorTone::blueTone:
				return tB;

			case ColorTone::purpleTone:
				return tP;

			case ColorTone::cyanTone:
				return tC;

			case ColorTone::oliveTone:
				return tO;

			case ColorTone::customizedTone:
				return tCustom;

			default:
				return tDefault;
		}
	}

	Options configuredOptions(bool bEnable)
	{
		Options opt;
		opt.enable = bEnable;
		opt.enableMenubar = opt.enable;

		g_colorToneChoice = ColorTone::blackTone; // black tone
		tCustom.change(darkColors);
		_dpiManager.Invalidate();

		return opt;
	}

	bool g_initialized = false;
	void initDarkMode()
	{
		g_initialized = true;

		_options = configuredOptions(false);

		initExperimentalDarkMode();
		setDarkMode(false, true);
	}

	bool isInitialized() {
		return g_initialized;
	}

	bool isEnabled()
	{
		return _options.enable;
	}

	bool isDarkMenuEnabled()
	{
		return _options.enableMenubar;
	}

	bool isExperimentalActive()
	{
		return g_darkModeEnabled;
	}

	bool isExperimentalSupported()
	{
		return g_darkModeSupported;
	}

	bool isWindows11()
	{
		return IsWindows11();
	}

	COLORREF invertLightness(COLORREF c)
	{
		WORD h = 0;
		WORD s = 0;
		WORD l = 0;
		ColorRGBToHLS(c, &h, &l, &s);

		l = HLSMAXRANGE - l;

		COLORREF invert_c = ColorHLSToRGB(h, l, s);

		return invert_c;
	}

	COLORREF invertLightnessSofter(COLORREF c)
	{
		WORD h = 0;
		WORD s = 0;
		WORD l = 0;
		ColorRGBToHLS(c, &h, &l, &s);

		l = std::min(180 - l, 151);

		COLORREF invert_c = ColorHLSToRGB(h, l, s);

		return invert_c;
	}

	COLORREF lightColor(COLORREF color, WORD luminance)
	{
		WORD h = 0;
		WORD s = 0;
		WORD l = 0;
		ColorRGBToHLS(color, &h, &l, &s);

		l = luminance;

		COLORREF newColor = ColorHLSToRGB(h, l, s);

		return newColor;
	}

	// adapted from https://stackoverflow.com/a/56678483
	double calculatePerceivedLighness(COLORREF c)
	{
		auto linearValue = [](double colorChannel) -> double
		{
			colorChannel /= 255.0;
			if (colorChannel <= 0.04045)
				return colorChannel / 12.92;
			return std::pow(((colorChannel + 0.055) / 1.055), 2.4);
		};

		double r = linearValue(static_cast<double>(GetRValue(c)));
		double g = linearValue(static_cast<double>(GetGValue(c)));
		double b = linearValue(static_cast<double>(GetBValue(c)));

		double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;

		double lighness = (luminance <= 216.0 / 24389.0) ? (luminance * 24389.0 / 27.0) : (std::pow(luminance, (1.0 / 3.0)) * 116.0 - 16.0);
		return lighness;
	}

	COLORREF getBackgroundColor()         { return getTheme()._colors.background; }
	COLORREF getSofterBackgroundColor()   { return getTheme()._colors.softerBackground; }
	COLORREF getHotBackgroundColor()      { return getTheme()._colors.hotBackground; }
	COLORREF getDarkerBackgroundColor()   { return getTheme()._colors.pureBackground; }
	COLORREF getErrorBackgroundColor()    { return getTheme()._colors.errorBackground; }
	COLORREF getTextColor()               { return getTheme()._colors.text; }
	COLORREF getDarkerTextColor()         { return getTheme()._colors.darkerText; }
	COLORREF getDisabledTextColor()       { return getTheme()._colors.disabledText; }
	COLORREF getLinkTextColor()           { return getTheme()._colors.linkText; }
	COLORREF getEdgeColor()               { return getTheme()._colors.edge; }

	HBRUSH getBackgroundBrush()          { return getTheme()._brushes.background; }
	HBRUSH getSofterBackgroundBrush()    { return getTheme()._brushes.softerBackground; }
	HBRUSH getHotBackgroundBrush()       { return getTheme()._brushes.hotBackground; }
	HBRUSH getDarkerBackgroundBrush()    { return getTheme()._brushes.pureBackground; }
	HBRUSH getErrorBackgroundBrush()     { return getTheme()._brushes.errorBackground; }
	HBRUSH getHardlightBackgroundBrush() { return getTheme()._brushes.hardlightBackground; }
	HBRUSH getSoftlightBackgroundBrush() { return getTheme()._brushes.softlightBackground; }
	HBRUSH getTextBrush()				 { return getTheme()._brushes.textColorBrush; }
	HBRUSH getDarkerTextBrush()			 { return getTheme()._brushes.darkerTextColorBrush; }

	HPEN getDarkerTextPen()               { return getTheme()._pens.darkerTextPen; }
	HPEN getEdgePen()                     { return getTheme()._pens.edgePen; }
	HPEN getLightEdgePen()				  { return getTheme()._pens.lightEdgePen; }
	HPEN getDarkEdgePen()				  { return getTheme()._pens.darkEdgePen; }

	void setThemeColors(Colors& newColors)
	{
		getTheme().change(newColors);
	}

	void setBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.background = c;
		getTheme().change(clrs);
	}

	void setSofterBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.softerBackground = c;
		getTheme().change(clrs);
	}

	void setHotBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.hotBackground = c;
		getTheme().change(clrs);
	}

	void setDarkerBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.pureBackground = c;
		getTheme().change(clrs);
	}

	void setErrorBackgroundColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.errorBackground = c;
		getTheme().change(clrs);
	}

	void setTextColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.text = c;
		getTheme().change(clrs);
	}

	void setDarkerTextColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.darkerText = c;
		getTheme().change(clrs);
	}

	void setDisabledTextColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.disabledText = c;
		getTheme().change(clrs);
	}

	void setLinkTextColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.linkText = c;
		getTheme().change(clrs);
	}

	void setEdgeColor(COLORREF c)
	{
		Colors clrs = getTheme()._colors;
		clrs.edge = c;
		getTheme().change(clrs);
	}

	Colors getDarkModeDefaultColors()
	{
		return darkColors;
	}

	void changeCustomTheme(const Colors& colors)
	{
		tCustom.change(colors);
	}

	// handle events

	void handleSettingChange(HWND hwnd, LPARAM lParam)
	{
		UNREFERENCED_PARAMETER(hwnd);

		if (!isExperimentalSupported())
		{
			return;
		}

		if (IsColorSchemeChangeMessage(lParam))
		{
			g_darkModeEnabled = ShouldAppsUseDarkMode() && !IsHighContrast();
		}
	}

	// processes messages related to UAH / custom menubar drawing.
	// return true if handled, false to continue with normal processing in your wndproc
	bool runUAHWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam, LRESULT* lr)
	{
		static HTHEME g_menuTheme = nullptr;

		UNREFERENCED_PARAMETER(wParam);
		switch (message)
		{
		case WM_UAHDRAWMENU:
		{
			UAHMENU* pUDM = (UAHMENU*)lParam;
			RECT rc = {};

			// get the menubar rect
			{
				MENUBARINFO mbi = { sizeof(mbi) };
				GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi);

				RECT rcWindow;
				GetWindowRect(hWnd, &rcWindow);

				// the rcBar is offset by the window rect
				rc = mbi.rcBar;
				OffsetRect(&rc, -rcWindow.left, -rcWindow.top);

				rc.top -= 1;
			}

			FillRect(pUDM->hdc, &rc, PluginDarkMode::getDarkerBackgroundBrush());

			*lr = 0;

			return true;
		}
		case WM_UAHDRAWMENUITEM:
		{
			UAHDRAWMENUITEM* pUDMI = (UAHDRAWMENUITEM*)lParam;

			// get the menu item string
			wchar_t menuString[256] = { '\0' };
			MENUITEMINFO mii = { sizeof(mii), MIIM_STRING };
			{
				mii.dwTypeData = menuString;
				mii.cch = (sizeof(menuString) / 2) - 1;

				GetMenuItemInfo(pUDMI->um.hmenu, pUDMI->umi.iPosition, TRUE, &mii);
			}

			// get the item state for drawing

			DWORD dwFlags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;

			int iTextStateID = MPI_NORMAL;
			int iBackgroundStateID = MPI_NORMAL;
			{
				if ((pUDMI->dis.itemState & ODS_INACTIVE) | (pUDMI->dis.itemState & ODS_DEFAULT))
				{
					// normal display
					iTextStateID = MPI_NORMAL;
					iBackgroundStateID = MPI_NORMAL;
				}
				if (pUDMI->dis.itemState & ODS_HOTLIGHT)
				{
					// hot tracking
					iTextStateID = MPI_HOT;
					iBackgroundStateID = MPI_HOT;
				}
				if (pUDMI->dis.itemState & ODS_SELECTED)
				{
					// clicked -- MENU_POPUPITEM has no state for this, though MENU_BARITEM does
					iTextStateID = MPI_HOT;
					iBackgroundStateID = MPI_HOT;
				}
				if ((pUDMI->dis.itemState & ODS_GRAYED) || (pUDMI->dis.itemState & ODS_DISABLED))
				{
					// disabled / grey text
					iTextStateID = MPI_DISABLED;
					iBackgroundStateID = MPI_DISABLED;
				}
				if (pUDMI->dis.itemState & ODS_NOACCEL)
				{
					dwFlags |= DT_HIDEPREFIX;
				}
			}

			if (!g_menuTheme)
			{
				g_menuTheme = OpenThemeData(hWnd, L"Menu");
			}

			if (iBackgroundStateID == MPI_NORMAL || iBackgroundStateID == MPI_DISABLED)
			{
				FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, PluginDarkMode::getDarkerBackgroundBrush());
			}
			else if (iBackgroundStateID == MPI_HOT || iBackgroundStateID == MPI_DISABLEDHOT)
			{
				FillRect(pUDMI->um.hdc, &pUDMI->dis.rcItem, PluginDarkMode::getHotBackgroundBrush());
			}
			else
			{
				DrawThemeBackground(g_menuTheme, pUDMI->um.hdc, MENU_POPUPITEM, iBackgroundStateID, &pUDMI->dis.rcItem, nullptr);
			}
			DTTOPTS dttopts = { sizeof(dttopts) };
			if (iTextStateID == MPI_NORMAL || iTextStateID == MPI_HOT)
			{
				dttopts.dwFlags |= DTT_TEXTCOLOR;
				dttopts.crText = PluginDarkMode::getTextColor();
			}
			DrawThemeTextEx(g_menuTheme, pUDMI->um.hdc, MENU_POPUPITEM, iTextStateID, menuString, mii.cch, dwFlags, &pUDMI->dis.rcItem, &dttopts);

			*lr = 0;

			return true;
		}
		case WM_THEMECHANGED:
		{
			if (g_menuTheme)
			{
				CloseThemeData(g_menuTheme);
				g_menuTheme = nullptr;
			}
			// continue processing in main wndproc
			return false;
		}
		default:
			return false;
		}
	}

	void drawUAHMenuNCBottomLine(HWND hWnd)
	{
		MENUBARINFO mbi = { sizeof(mbi) };
		if (!GetMenuBarInfo(hWnd, OBJID_MENU, 0, &mbi))
		{
			return;
		}

		RECT rcClient = {};
		GetClientRect(hWnd, &rcClient);
		MapWindowPoints(hWnd, nullptr, (POINT*)&rcClient, 2);

		RECT rcWindow = {};
		GetWindowRect(hWnd, &rcWindow);

		OffsetRect(&rcClient, -rcWindow.left, -rcWindow.top);

		// the rcBar is offset by the window rect
		RECT rcAnnoyingLine = rcClient;
		rcAnnoyingLine.bottom = rcAnnoyingLine.top;
		rcAnnoyingLine.top--;


		HDC hdc = GetWindowDC(hWnd);
		FillRect(hdc, &rcAnnoyingLine, PluginDarkMode::getDarkerBackgroundBrush());
		ReleaseDC(hWnd, hdc);
	}

	// from DarkMode.h

	void initExperimentalDarkMode()
	{
		::InitDarkMode();
	}

	void setDarkMode(bool useDark, bool fixDarkScrollbar)
	{
		_options.enable = useDark;
		_options.enableMenubar = useDark;
		::SetDarkMode(useDark, fixDarkScrollbar);
	}

	void allowDarkModeForApp(bool allow)
	{
		::AllowDarkModeForApp(allow);
	}

	bool allowDarkModeForWindow(HWND hWnd, bool allow)
	{
		return ::AllowDarkModeForWindow(hWnd, allow);
	}

	void setTitleBarThemeColor(HWND hWnd)
	{
		::RefreshTitleBarThemeColor(hWnd);
	}

	void enableDarkScrollBarForWindowAndChildren(HWND hwnd)
	{
		::EnableDarkScrollBarForWindowAndChildren(hwnd);
	}

	bool colorizeBitmap(HBITMAP image, WORD h, WORD l, WORD s,
		bool changeH, bool changeL, bool changeS, bool testAlphaForChange,
		short rotateH, short displaceL, short displaceS, bool testAlphaForDisplace,
		bool invertR, bool invertG, bool invertB, bool testAlphaForInvertColor,
		bool invertLight, WORD invertLightCeiling, WORD invertLightFloor, bool testAlphaForInvertLight,
		WORD luminosityMin, WORD luminosityMax, bool useLuminosityMin, bool useLuminosityMax, bool testAlphaForLuminosityThreshold,
		bool alphaUnpremultiply)
	{
		// Get bitmap info and bits
		struct { BITMAPINFO info = {}; RGBQUAD moreColors[255]; } bmi;
		BITMAPINFOHEADER& bmh = bmi.info.bmiHeader;
		BITMAP bm = {};

		bmh.biSize = sizeof(bmh);
		GetObject(image, sizeof(bm), &bm);

		if (bm.bmBitsPixel != 32)
			return false;

		HDC memDC = CreateCompatibleDC(NULL);
		if (!memDC)
			return false;

		std::unique_ptr<std::uint8_t[]> bitmapData;
		if (bm.bmBits == NULL)
		{
			GetDIBits(memDC, image, 0, bm.bmHeight, NULL, &bmi.info, DIB_RGB_COLORS);
			bitmapData = std::make_unique<std::uint8_t[]>(bmh.biSizeImage);
			int scanCopied = 0;
			scanCopied = GetDIBits(memDC, image, 0, bm.bmHeight, bitmapData.get(), &bmi.info, DIB_RGB_COLORS);

			if (scanCopied < bm.bmHeight)
			{
				DeleteDC(memDC);
				return false;
			}
		}

		// Map bitmap memory
		WORD srcH, srcL, srcS;
		std::uint8_t r, g, b, a;
		std::uint8_t* data, * initRowData, * rowData = bm.bmBits == NULL ? bitmapData.get() : static_cast<std::uint8_t*>(bm.bmBits);

		DWORD stride = bm.bmWidthBytes;
		COLORREF colorResult = 0;

		initRowData = rowData;
		std::uint32_t bmHeight = static_cast<std::uint32_t>(bm.bmHeight);
		std::uint32_t bmWidth = static_cast<std::uint32_t>(bm.bmWidth);
		DWORD index = 0;

		// Image processing
		for (std::uint32_t y = 0; y < bmHeight; y++)
		{
			data = rowData;
			for (std::uint32_t x = 0; x < bmWidth; x++)
			{
				// Step 1 - Conversion
				a = data[3];
				r = data[2];
				g = data[1];
				b = data[0];

				if (alphaUnpremultiply && a != 0)
				{
					r = (r * 255) / a;
					g = (g * 255) / a;
					b = (b * 255) / a;
				}

				ColorRGBToHLS(RGB(r, g, b), &srcH, &srcL, &srcS);

				// Step 2 - Modification
				if (!testAlphaForChange || (testAlphaForChange && a > 0))
				{
					if (changeH)
						srcH = h;
					if (changeL)
						srcL = l;
					if (changeS)
						srcS = s;
				}
				if (!testAlphaForDisplace || (testAlphaForDisplace && a > 0))
				{
					if (rotateH != 0) // avoid unecessary large calcs
						srcH = (((srcH + rotateH) / HLSMAXRANGE) * HLSMAXRANGE) + ((srcH + rotateH) % HLSMAXRANGE);
					if (displaceL != 0)
						srcL = displaceL > 0 ? std::min(HLSMAXRANGE, srcL + displaceL) : std::max(0, srcL - displaceL);
					if (displaceS != 0)
						srcS = displaceS > 0 ? std::min(HLSMAXRANGE, srcS + displaceS) : std::max(0, srcS - displaceS);;
				}
				if (invertLight)
				{
					if (!testAlphaForInvertColor || (testAlphaForInvertColor && a > 0))
					{
						srcL = std::min(invertLightCeiling - srcL, static_cast<int>(invertLightFloor));
					}
				}
				if (!testAlphaForLuminosityThreshold || (testAlphaForLuminosityThreshold && a > 0))
				{
					if (useLuminosityMin)
						srcL = std::max(srcL, luminosityMin);
					if (useLuminosityMax)
						srcL = std::min(srcL, luminosityMax);
				}

				// Step 3 - Sanitization
				srcH = srcH < 0 ? 0 : (srcH > 240 ? 240 : srcH);
				srcL = srcL < 0 ? 0 : (srcL > 240 ? 240 : srcL);
				srcS = srcS < 0 ? 0 : (srcS > 240 ? 240 : srcS);

				// Step 4 - back cast
				colorResult = ColorHLSToRGB(srcH, srcL, srcS);
				data[2] = GetRValue(colorResult);
				data[1] = GetGValue(colorResult);
				data[0] = GetBValue(colorResult);

				// Step 5 - Color inversion
				if (!testAlphaForInvertColor || (testAlphaForInvertColor && a > 0))
				{
					if (invertR)
						data[2] = 255 - data[2];
					if (invertG)
						data[1] = 255 - data[1];
					if (invertB)
						data[0] = 255 - data[0];
				}

				data += 4;
			}
			rowData += stride;
		}

		// Modify bitmap bits
		int copied = 0;
		copied = SetDIBits(memDC, image, 0, bm.bmHeight, initRowData, &bmi.info, DIB_RGB_COLORS);
		DeleteDC(memDC);

		if (copied < bm.bmHeight)
			return false;
		else
			return true;
	}

	HBITMAP createCustomThemeBackgroundBitmap(HTHEME hTheme, int iPartID, int iStateID, WORD extraLuminance)
	{
		HDC screenDC = GetDC(NULL);
		HDC hdc = CreateCompatibleDC(screenDC);
		SIZE sSize;
		HRESULT hSuccess = GetThemePartSize(hTheme, hdc, iPartID, iStateID, NULL, THEMESIZE::TS_DRAW, &sSize);

		if (FAILED(hSuccess))
			return NULL;

		std::unique_ptr<std::uint8_t[]> bmpBits = std::make_unique< std::uint8_t[]>(sSize.cx * sSize.cy * 4);
		ZeroMemory(bmpBits.get(), sSize.cx * sSize.cy * 4);

		HBITMAP hReturn = CreateBitmap(sSize.cx, sSize.cy, 1, 32, bmpBits.get());
		HBITMAP hOldBitmap = SelectBitmap(hdc, hReturn);

		RECT rcBitmap = { 0, 0, sSize.cx, sSize.cy };
		DrawThemeBackground(hTheme, hdc, iPartID, iStateID, &rcBitmap, NULL);

		SelectObject(hdc, hOldBitmap);
		DeleteDC(hdc);

		HLS backColors;
		ColorRGBToHLS(getBackgroundColor(), &backColors.h, &backColors.l, &backColors.s);
		bool bsuccess = colorizeBitmap(hReturn, backColors.h, backColors.l,	backColors.s, true, false, true, true, 0, 
			extraLuminance, 0);
		if (bsuccess)
			return hReturn;

		return NULL;
	}

	enum class SpinButtonType { LeftRight, UpDown };
	enum class SpinArrowDirection { Left, Right, Up, Down };

	struct ButtonData
	{
		HTHEME hTheme = nullptr;
		int iStateID = 0;

		~ButtonData()
		{
			closeTheme();
		}

		bool ensureTheme(HWND hwnd)
		{
			if (!hTheme)
			{
				hTheme = OpenThemeData(hwnd, L"Button");
			}
			return hTheme != nullptr;
		}

		void closeTheme()
		{
			if (hTheme)
			{
				CloseThemeData(hTheme);
				hTheme = nullptr;
			}
		}
	};

	struct HeaderItemData
	{
		HTHEME hTheme = nullptr;
		HBITMAP upArrow[2] = {};
		HBITMAP downArrow[2] = {};
		bool lastDropDownTrackWasHot = false;

		~HeaderItemData()
		{
			closeTheme();
		}

		bool ensureTheme()
		{
			if (!hTheme)
			{
				hTheme = OpenThemeData(nullptr, L"Header");
				reloadStructBitmaps();
			}
			return hTheme != nullptr;
		}

		void reloadStructBitmaps()
		{
			if (hTheme)
			{
				upArrow[0] = createCustomThemeBackgroundBitmap(hTheme, HP_HEADERSORTARROW, HSAS_SORTEDUP);
				upArrow[1] = createCustomThemeBackgroundBitmap(hTheme, HP_HEADERSORTARROW, HSAS_SORTEDUP, 60);
				downArrow[0] = createCustomThemeBackgroundBitmap(hTheme, HP_HEADERSORTARROW, HSAS_SORTEDDOWN);
				downArrow[1] = createCustomThemeBackgroundBitmap(hTheme, HP_HEADERSORTARROW, HSAS_SORTEDDOWN, 60);
			}
		}

		void closeTheme()
		{
			if (hTheme)
			{
				CloseThemeData(hTheme);
				hTheme = nullptr;
				DeleteObject(upArrow[0]);
				DeleteObject(upArrow[1]);
				DeleteObject(downArrow[0]);
				DeleteObject(downArrow[1]);
			}
		}

		void drawSortArrow(HDC destDC, int iStateID, const RECT& clientRect)
		{
			drawArrow(destDC, iStateID, clientRect, false, false);
		}

		void drawComboArrow(HDC destDC, const RECT& comboRect, bool Hot)
		{
			drawArrow(destDC, HSAS_SORTEDDOWN, comboRect, true, Hot);
		}

	private:

		void drawArrow(HDC destDC, int iStateID, const RECT& clientRect, bool bComboArrow, bool Hot)
		{
			RECT rcArrow = {};
			BITMAP bp;
			if (iStateID == HSAS_SORTEDUP)
				GetObject(upArrow[0], sizeof(bp), &bp);
			else
				GetObject(downArrow[0], sizeof(bp), &bp);

			rcArrow.top = bComboArrow ? (clientRect.bottom - bp.bmHeight) / 2 : 1;
			rcArrow.left = clientRect.left + ((clientRect.right - clientRect.left - bp.bmWidth) / 2);
			rcArrow.right = rcArrow.left + bp.bmWidth;
			rcArrow.bottom = rcArrow.top + bp.bmHeight;

			HBITMAP toDraw = (iStateID == HSAS_SORTEDUP) ? (Hot ? upArrow[1] : upArrow[0]) : (Hot ? downArrow[1] : downArrow[0]);
			drawGlyph(destDC, rcArrow, toDraw, { bp.bmWidth, bp.bmHeight });
		}

		void drawGlyph(HDC hdc, const RECT& finalRect, const HBITMAP glyph, const SIZE& glyphSize)
		{
			HDC srcDC = CreateCompatibleDC(NULL);
			HBITMAP hOldBitmap = SelectBitmap(srcDC, glyph);
			BLENDFUNCTION bf1;
			bf1.BlendOp = AC_SRC_OVER;
			bf1.BlendFlags = 0;
			bf1.SourceConstantAlpha = 0xff;
			bf1.AlphaFormat = AC_SRC_ALPHA;
			GdiAlphaBlend(hdc, finalRect.left, finalRect.top, glyphSize.cx, glyphSize.cy, srcDC, 0, 0, glyphSize.cx, glyphSize.cy, bf1);
			SelectBitmap(srcDC, hOldBitmap);
			DeleteDC(srcDC);
		}
	};

	struct SpinData
	{
		HWND hWnd = nullptr;
		HTHEME hTheme = nullptr;
		SpinButtonType type = SpinButtonType::LeftRight;
		RECT currentRcClient = {};
		int iStateID1 = 0;
		int iStateID2 = 0;

		HBITMAP SpinArrowOne[4] = {};
		HBITMAP SpinArrowTwo[4] = {};

		~SpinData()
		{
			closeTheme();
		}

		bool ensureTheme(HWND hwnd)
		{
			RECT rcClient;
			GetClientRect(hwnd, &rcClient);
			if (!hTheme)
			{
				hTheme = OpenThemeData(hwnd, L"Spin");

				if (rcClient.right - rcClient.left > rcClient.bottom - rcClient.top)
					type = SpinButtonType::LeftRight;
				else
					type = SpinButtonType::UpDown;
			}

			reloadImages(rcClient);

			return hTheme != nullptr;
		}

		void closeTheme()
		{
			if (hTheme)
			{
				CloseThemeData(hTheme);
				DeleteObject(SpinArrowOne);
				DeleteObject(SpinArrowTwo);
				hTheme = nullptr;
			}
		}

	private:
		// Spin controls almost never resize, so loading bitmaps once for it is cheap and speeds draws later
		void reloadImages(const RECT& rcClient)
		{
			if (!hTheme || EqualRect(&rcClient, &currentRcClient))
				return;

			int cx = rcClient.right - rcClient.left;
			int cy = rcClient.bottom - rcClient.top;
			HDC newDC = CreateCompatibleDC(NULL);
			HBITMAP hOldBitmap = nullptr;

			RECT spinRect = {};
			HLS controlBack;
			HLS colorDarkText;
			ColorRGBToHLS(getBackgroundColor(), &controlBack.h, &controlBack.l, &controlBack.s);
			ColorRGBToHLS(getDarkerTextColor(), &colorDarkText.h, &colorDarkText.l, &colorDarkText.s);
			int internalStatus = 0;
			for (int i = 0; i < 4; i++)
			{
				internalStatus = i + 1;

				// Create bitmaps
				SpinArrowOne[i] = CreateBitmap(type == SpinButtonType::LeftRight ? cx / 2 : cx, cy, 1, 32, NULL);
				SpinArrowTwo[i] = CreateBitmap(cx, type == SpinButtonType::UpDown ? cy / 2 : cy, 1, 32, NULL);

				GetThemeBackgroundExtent(hTheme, newDC, SPNP_UPHORZ, i + 1, &rcClient, &spinRect);

				if (type == SpinButtonType::LeftRight)
					spinRect.right /= 2;
				else
					spinRect.bottom /= 2;

				if (hOldBitmap == nullptr)
					hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject(newDC, SpinArrowOne[i]));
				else
					SelectObject(newDC, SpinArrowOne[i]);

				// Draw theme buttons
				// SPNP_DOWNHORZ = left arrow, SPNP_UPHORZ = right arrow
				RECT arrowRc = { 0, 0, cx, cy };
				FillRect(newDC, &arrowRc, getBackgroundBrush());
				DrawThemeBackground(hTheme, newDC, type == SpinButtonType::LeftRight ? SPNP_DOWNHORZ : SPNP_UP, i + 1, &spinRect, NULL);
				SelectObject(newDC, SpinArrowTwo[i]);
				FillRect(newDC, &arrowRc, getBackgroundBrush());
				DrawThemeBackground(hTheme, newDC, type == SpinButtonType::LeftRight ? SPNP_UPHORZ : SPNP_DOWN, i + 1, &spinRect, NULL);

				if (hOldBitmap)
					SelectObject(newDC, hOldBitmap);

				// How to turn a "light themed" control into a Dark Mode one?

				// Step 1 -> Invert colors
				colorizeBitmap(SpinArrowOne[i], 0, 0, 0, false, false, false, true, 0, 0, 0, true, true, true, true);
				colorizeBitmap(SpinArrowTwo[i], 0, 0, 0, false, false, false, true, 0, 0, 0, true, true, true, true);

				// Step 2 -> Attenuate minimum luminosity to control background's light.
				colorizeBitmap(SpinArrowOne[i], 0, 0, 0, false, false, false, true, 0, 0, 0, true, 
					false, false, false, true, false, 240, 0, true, controlBack.l, 240, true);
				colorizeBitmap(SpinArrowTwo[i], 0, 0, 0, false, false, false, true, 0, 0, 0, true,
					false, false, false, true, false, 240, 0, true, controlBack.l, 240, true);

				// Step 3 -> Attenuate maximum luminosity to control light - for statuses UPS_NORMAL (1) and UPS_DISABLED (4)
				if (internalStatus == UPS_NORMAL || internalStatus == UPS_DISABLED)
				{
					colorizeBitmap(SpinArrowOne[i], 0, 0, 0, false, false, false, true, 0, 0, 0, true,
						false, false, false, true, false, 240, 0, true, 0, colorDarkText.l, false, true);
					colorizeBitmap(SpinArrowTwo[i], 0, 0, 0, false, false, false, true, 0, 0, 0, true,
						false, false, false, true, false, 240, 0, true, 0, colorDarkText.l, false, true);
				}

				// Step 4 -> Colorize to theme
				colorizeBitmap(SpinArrowOne[i], controlBack.h, 0, controlBack.s, true, false, true);
				colorizeBitmap(SpinArrowTwo[i], controlBack.h, 0, controlBack.s, true, false, true);
			}

			currentRcClient = rcClient;
			DeleteDC(newDC);
		}
	};

	// Draws a BS_PUSBUTON or DEF_PUSHBUTTON or Checkbox with BS_PUSHLIKE control background
	// nState is the same as static_cast<DWORD>(SendMessage(hwndButton, BM_GETSTATE, 0, 0));
	// nStyle is the same as GetWindowLongPtr(hwndButton, GWL_STYLE);
	void renderButtonBackground(HDC hdc, DWORD nState, LONG_PTR nStyle, const RECT& rcClient)
	{
		HBRUSH hBckBrush = ((nState & BST_HOT) != 0) ? PluginDarkMode::getSoftlightBackgroundBrush() : PluginDarkMode::getDarkerBackgroundBrush();
		if ((nState & BST_PUSHED) != 0 || ((nState & BST_CHECKED) != 0))
			hBckBrush = PluginDarkMode::getSofterBackgroundBrush();

		HPEN hOldPen = nullptr;
		if (nStyle & WS_DISABLED)
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, PluginDarkMode::getDarkEdgePen()));
		else if ((nState & (BST_FOCUS) | (nState & BST_HOT)) || ((nStyle & BS_DEFPUSHBUTTON) && !(nStyle & BS_PUSHLIKE)))
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, PluginDarkMode::getLightEdgePen()));
		else if (nStyle & (BS_PUSHLIKE))
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, PluginDarkMode::getEdgePen()));
		else
			hOldPen = reinterpret_cast<HPEN>(SelectObject(hdc, PluginDarkMode::getEdgePen()));

		HBRUSH hOldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, hBckBrush));
		RoundRect(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, _dpiManager.scaleX(5), _dpiManager.scaleY(5));
		SelectObject(hdc, hOldBrush);

		if (hOldPen)
			SelectObject(hdc, hOldPen);
	}

	void renderButton(HWND hwndButton, HDC hdc, HTHEME hTheme)
	{
		RECT rcClient = {};
		WCHAR szText[256] = { '\0' };
		DWORD nState = static_cast<DWORD>(SendMessage(hwndButton, BM_GETSTATE, 0, 0));
		LONG_PTR nStyle = GetWindowLongPtr(hwndButton, GWL_STYLE);
		DWORD uiState = static_cast<DWORD>(SendMessage(hwndButton, WM_QUERYUISTATE, 0, 0));

		GetClientRect(hwndButton, &rcClient);
		GetWindowText(hwndButton, szText, _countof(szText));

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;

		renderButtonBackground(hdc, nState, nStyle, rcClient);

		// Draw button image
		RECT rcImage = rcClient;
		RECT rcText = rcClient;

		// Calculate actual text output rectangle and centralize
		const int padding = _dpiManager.scaleX(4);
		DrawText(hdc, szText, sizeof(szText), &rcImage, DT_CALCRECT);
		rcImage.left = padding + (rcClient.right - rcImage.right) / 2;
		rcImage.right += padding + rcImage.left;

		ICONINFO ii;
		BITMAP bm;

		HICON hIcon = reinterpret_cast<HICON>(SendMessage(hwndButton, BM_GETIMAGE, IMAGE_ICON, 0));
		HBITMAP hBitmap = reinterpret_cast<HBITMAP>(hIcon); // BM_GETIMAGE returns the same handler for IMAGE_ICON and IMAGE_BITMAP.
		BOOL bIcon = GetIconInfo(hIcon, &ii);
		BOOL bBitmap = GetObject(hBitmap, sizeof(bm), &bm);

		bool bStandalone = ((nStyle & BS_BITMAP) != 0) || ((nStyle & BS_ICON) != 0) || (szText[0] == '\0');

		if (bIcon)
		{
			POINT pxIcon = {};
			rcImage.left -= ii.xHotspot * 2;
			pxIcon.x = bStandalone ? (rcClient.right - ii.xHotspot * 2) / 2 : rcImage.left;
			pxIcon.y = (rcClient.bottom - (ii.yHotspot * 2)) / 2;
			if (nState & BST_PUSHED)
			{
				pxIcon.x += _dpiManager.scaleX(1);
				pxIcon.y += _dpiManager.scaleY(1);
			}
			DrawIconEx(hdc, pxIcon.x, pxIcon.y, hIcon, ii.xHotspot * 2, ii.yHotspot * 2, 0, NULL, DI_NORMAL);
		}

		if (bBitmap)
		{
			POINT pxBmp = {};
			rcImage.left -= bm.bmWidth;
			HDC memDC = CreateCompatibleDC(hdc);
			pxBmp.x = bStandalone ? (rcClient.right - bm.bmWidth) / 2 : rcImage.left;
			pxBmp.y = (rcClient.bottom - bm.bmHeight) / 2;
			if (nState & BST_PUSHED)
			{
				pxBmp.x += _dpiManager.scaleX(1);
				pxBmp.y += _dpiManager.scaleY(1);
			}

			HBITMAP oldBmp = reinterpret_cast<HBITMAP>(SelectObject(memDC, hBitmap));
			if (bm.bmBitsPixel == 32)
			{
				BLENDFUNCTION bf1;
				bf1.BlendOp = AC_SRC_OVER;
				bf1.BlendFlags = 0;
				bf1.SourceConstantAlpha = 0xff;
				bf1.AlphaFormat = AC_SRC_ALPHA;
				GdiAlphaBlend(hdc, pxBmp.x, pxBmp.y, bm.bmWidth, bm.bmHeight, memDC, 0, 0, bm.bmWidth, bm.bmHeight, bf1);
			}
			else
				BitBlt(hdc, pxBmp.x, pxBmp.y, bm.bmWidth, bm.bmHeight, memDC, 0, 0, SRCCOPY);

			SelectObject(memDC, oldBmp);
			DeleteDC(memDC);
		}

		if (bIcon || bBitmap)
			rcText.left += padding;

		hFont = reinterpret_cast<HFONT>(SendMessage(hwndButton, WM_GETFONT, 0, 0));
		hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));


		DTTOPTS dtto = { sizeof(DTTOPTS), DTT_TEXTCOLOR };
		dtto.crText = PluginDarkMode::getTextColor();

		if (nStyle & WS_DISABLED)
		{
			dtto.crText = PluginDarkMode::getDisabledTextColor();
		}

		if (nState & BST_PUSHED)
		{
			rcText.left += _dpiManager.scaleX(1);
			rcText.right += _dpiManager.scaleX(1);
			rcText.top += _dpiManager.scaleY(1);
			rcText.bottom += _dpiManager.scaleY(1);
		}

		DWORD dtFlags = DT_LEFT; // DT_LEFT is 0
		dtFlags |= (nStyle & BS_MULTILINE) ? DT_WORDBREAK : DT_SINGLELINE;
		dtFlags |= ((nStyle & BS_CENTER) == BS_CENTER) ? DT_CENTER : (nStyle & BS_RIGHT) ? DT_RIGHT : 0;
		dtFlags |= ((nStyle & BS_VCENTER) == BS_VCENTER) ? DT_VCENTER : (nStyle & BS_BOTTOM) ? DT_BOTTOM : 0;
		dtFlags |= (uiState & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0;

		// Modifications to DrawThemeText
		dtFlags &= ~(DT_RIGHT);
		dtFlags |= DT_VCENTER | DT_CENTER;

		int iStateID = PBS_NORMAL;
		if (nStyle & WS_DISABLED)				iStateID = PBS_DISABLED;
		else if (nState & BST_PUSHED)			iStateID = PBS_PRESSED;
		else if (nState & BST_HOT)				iStateID = PBS_HOT;
		else if (nStyle & BS_DEFPUSHBUTTON)		iStateID = PBS_DEFAULTED;

		DrawThemeTextEx(hTheme, hdc, BP_PUSHBUTTON, iStateID, szText, -1, dtFlags, &rcText, &dtto);

		if ((nState & BST_FOCUS) && !(uiState & UISF_HIDEFOCUS))
		{
			rcClient.left += _dpiManager.scaleX(2); rcClient.right -= _dpiManager.scaleX(2);
			rcClient.top += _dpiManager.scaleY(2); rcClient.bottom -= _dpiManager.scaleY(2);
			DrawFocusRect(hdc, &rcClient);
		}

		SelectObject(hdc, hOldFont);
	}

	void renderCheckboxOrRadioButton(HWND hwnd, HDC hdc, HTHEME hTheme, int iPartID, int iStateID)
	{
		RECT rcClient = {};
		WCHAR szText[256] = { '\0' };
		DWORD nState = static_cast<DWORD>(SendMessage(hwnd, BM_GETSTATE, 0, 0));
		DWORD uiState = static_cast<DWORD>(SendMessage(hwnd, WM_QUERYUISTATE, 0, 0));
		DWORD nStyle = GetWindowLong(hwnd, GWL_STYLE);

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;
		HFONT hCreatedFont = nullptr;
		LOGFONT lf = {};
		if (SUCCEEDED(GetThemeFont(hTheme, hdc, iPartID, iStateID, TMT_FONT, &lf)))
		{
			hCreatedFont = CreateFontIndirect(&lf);
			hFont = hCreatedFont;
		}

		if (!hFont) {
			hFont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
		}

		hOldFont = static_cast<HFONT>(SelectObject(hdc, hFont));

		DWORD dtFlags = DT_LEFT; // DT_LEFT is 0
		dtFlags |= (nStyle & BS_MULTILINE) ? DT_WORDBREAK : DT_SINGLELINE;
		dtFlags |= ((nStyle & BS_CENTER) == BS_CENTER) ? DT_CENTER : (nStyle & BS_RIGHT) ? DT_RIGHT : 0;
		dtFlags |= ((nStyle & BS_VCENTER) == BS_VCENTER) ? DT_VCENTER : (nStyle & BS_BOTTOM) ? DT_BOTTOM : 0;
		dtFlags |= (uiState & UISF_HIDEACCEL) ? DT_HIDEPREFIX : 0;

		if (!(nStyle & BS_MULTILINE) && !(nStyle & BS_BOTTOM) && !(nStyle & BS_TOP))
		{
			dtFlags |= DT_VCENTER;
		}

		GetClientRect(hwnd, &rcClient);
		GetWindowText(hwnd, szText, _countof(szText));

		SIZE szBox = { 13, 13 };
		GetThemePartSize(hTheme, hdc, iPartID, iStateID, NULL, TS_DRAW, &szBox);

		RECT rcText = rcClient;
		GetThemeBackgroundContentRect(hTheme, hdc, iPartID, iStateID, &rcClient, &rcText);

		RECT rcBackground = rcClient;
		if (dtFlags & DT_SINGLELINE)
		{
			rcBackground.top += (rcText.bottom - rcText.top - szBox.cy) / 2;
		}
		rcBackground.bottom = rcBackground.top + szBox.cy;
		rcBackground.right = rcBackground.left + szBox.cx;
		rcText.left = rcBackground.right + 3;

		DrawThemeParentBackground(hwnd, hdc, &rcClient);
		DrawThemeBackground(hTheme, hdc, iPartID, iStateID, &rcBackground, nullptr);

		DTTOPTS dtto = { sizeof(DTTOPTS), DTT_TEXTCOLOR };
		dtto.crText = PluginDarkMode::getTextColor();

		if (nStyle & WS_DISABLED)
		{
			dtto.crText = PluginDarkMode::getDisabledTextColor();
		}

		DrawThemeTextEx(hTheme, hdc, iPartID, iStateID, szText, -1, dtFlags, &rcText, &dtto);

		if ((nState & BST_FOCUS) && !(uiState & UISF_HIDEFOCUS))
		{
			RECT rcTextOut = rcText;
			dtto.dwFlags |= DTT_CALCRECT;
			DrawThemeTextEx(hTheme, hdc, iPartID, iStateID, szText, -1, dtFlags | DT_CALCRECT, &rcTextOut, &dtto);
			RECT rcFocus = rcTextOut;
			rcFocus.bottom++;
			rcFocus.left--;
			rcFocus.right++;
			DrawFocusRect(hdc, &rcFocus);
		}

		if (hCreatedFont) DeleteObject(hCreatedFont);
		SelectObject(hdc, hOldFont);
	}

	void paintButton(HWND hwnd, HDC hdc, ButtonData& buttonData)
	{
		DWORD nState = static_cast<DWORD>(SendMessage(hwnd, BM_GETSTATE, 0, 0));
		LONG_PTR nStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		DWORD nButtonStyle = nStyle & 0xF;

		int iPartID = 0;
		if (nButtonStyle == BS_PUSHBUTTON || nButtonStyle == BS_DEFPUSHBUTTON || (nStyle & BS_PUSHLIKE) > 0)
		{
			iPartID = BP_PUSHBUTTON;
		}
		else if (nButtonStyle == BS_RADIOBUTTON || nButtonStyle == BS_AUTORADIOBUTTON)
		{
			iPartID = BP_RADIOBUTTON;
		}
		else if (nButtonStyle == BS_CHECKBOX || nButtonStyle == BS_AUTOCHECKBOX)
		{
			iPartID = BP_CHECKBOX;
		}
		else
		{
			assert(false);
		}

		int iStateID = 0;
		
		// states of BP_CHECKBOX, BP_RADIOBUTTON and BP_PUSHBUTTON are the same
		iStateID = RBS_UNCHECKEDNORMAL;

		if (nStyle & WS_DISABLED)		iStateID = RBS_UNCHECKEDDISABLED;
		else if (nState & BST_PUSHED)	iStateID = RBS_UNCHECKEDPRESSED;
		else if (nState & BST_HOT)		iStateID = RBS_UNCHECKEDHOT;

		if (nState & BST_CHECKED)		iStateID += 4;

		if (BufferedPaintRenderAnimation(hwnd, hdc))
		{
			return;
		}

		BP_ANIMATIONPARAMS animParams = { sizeof(animParams) };
		animParams.style = BPAS_LINEAR;
		if (iStateID != buttonData.iStateID)
		{
			GetThemeTransitionDuration(buttonData.hTheme, iPartID, buttonData.iStateID, iStateID, TMT_TRANSITIONDURATIONS, &animParams.dwDuration);
		}

		RECT rcClient = {};
		GetClientRect(hwnd, &rcClient);

		HDC hdcFrom = nullptr;
		HDC hdcTo = nullptr;
		HANIMATIONBUFFER hbpAnimation = BeginBufferedAnimation(hwnd, hdc, &rcClient, BPBF_COMPATIBLEBITMAP, nullptr, &animParams, &hdcFrom, &hdcTo);
		if (hbpAnimation)
		{
			if (hdcFrom)
			{
				if (iPartID == BP_PUSHBUTTON)
					renderButton(hwnd, hdcFrom, buttonData.hTheme);
				else
					renderCheckboxOrRadioButton(hwnd, hdcFrom, buttonData.hTheme, iPartID, buttonData.iStateID);
			}
			if (hdcTo)
			{
				if (iPartID == BP_PUSHBUTTON)
					renderButton(hwnd, hdcTo, buttonData.hTheme);
				else
					renderCheckboxOrRadioButton(hwnd, hdcTo, buttonData.hTheme, iPartID, iStateID);
			}

			buttonData.iStateID = iStateID;

			EndBufferedAnimation(hbpAnimation, TRUE);
		}
		else
		{
			if (iPartID == BP_PUSHBUTTON)
				renderButton(hwnd, hdc, buttonData.hTheme);
			else
				renderCheckboxOrRadioButton(hwnd, hdc, buttonData.hTheme, iPartID, iStateID);

			buttonData.iStateID = iStateID;
		}
	}

	constexpr UINT_PTR g_buttonSubclassID = 42;

	LRESULT CALLBACK ButtonSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		UNREFERENCED_PARAMETER(uIdSubclass);

		auto pButtonData = reinterpret_cast<ButtonData*>(dwRefData);

		switch (uMsg)
		{
		case WM_UPDATEUISTATE:
			if (HIWORD(wParam) & (UISF_HIDEACCEL | UISF_HIDEFOCUS))
			{
				InvalidateRect(hWnd, nullptr, FALSE);
			}
			break;
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, ButtonSubclass, g_buttonSubclassID);
			delete pButtonData;
			break;
		case WM_ERASEBKGND:
			if (PluginDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
			{
				return TRUE;
			}
			else
			{
				break;
			}
		case WM_THEMECHANGED:
			pButtonData->closeTheme();
			break;
		case WM_PRINTCLIENT:
		case WM_PAINT:
			if (PluginDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
			{
				PAINTSTRUCT ps = {};
				HDC hdc = reinterpret_cast<HDC>(wParam);
				if (!hdc)
				{
					hdc = BeginPaint(hWnd, &ps);
				}

					ULONG_PTR token = 0;
					Gdiplus::GdiplusStartupInput input = NULL;
					Gdiplus::GdiplusStartup(&token, &input, NULL);

					paintButton(hWnd, hdc, *pButtonData);

				if (ps.hdc)
				{
					EndPaint(hWnd, &ps);
				}

					if (token)
						Gdiplus::GdiplusShutdown(token);


					return 0;
				}
				else
				{
					break;
				}
			case WM_SIZE:
			case WM_DESTROY:
				BufferedPaintStopAllAnimations(hWnd);
				break;
			case WM_ENABLE:
				if (PluginDarkMode::isEnabled())
				{
					// skip the button's normal wndproc so it won't redraw out of wm_paint
					LRESULT lr = DefWindowProc(hWnd, uMsg, wParam, lParam);
					InvalidateRect(hWnd, nullptr, FALSE);
					return lr;
				}
				break;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassButtonControl(HWND hwnd)
	{
		DWORD_PTR pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData());
		SetWindowSubclass(hwnd, ButtonSubclass, g_buttonSubclassID, pButtonData);
	}

	constexpr UINT_PTR g_comboBoxSubclassID = 42;

	LRESULT CALLBACK ComboBoxSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR /*dwRefData*/
	)
	{
		switch (uMsg)
		{
		case WM_PAINT:
		{
			if (!PluginDarkMode::isEnabled())
			{
				break;
			}

			RECT rc = {};
			::GetClientRect(hWnd, &rc);

			PAINTSTRUCT ps;
			auto hdc = ::BeginPaint(hWnd, &ps);

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, PluginDarkMode::getEdgePen()));
			::SelectObject(hdc, reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0)));
			::SetBkColor(hdc, PluginDarkMode::getBackgroundColor());

			::SelectObject(hdc, ::GetStockObject(NULL_BRUSH)); // to avoid text flicker, use only border
			::Rectangle(hdc, 0, 0, rc.right, rc.bottom);

			auto holdBrush = ::SelectObject(hdc, PluginDarkMode::getDarkerBackgroundBrush());

			RECT arrowRc = {
			rc.right - _dpiManager.scaleX(17), rc.top + 1,
			rc.right - 1, rc.bottom - 1
			};

			// CBS_DROPDOWN text is handled by parent by WM_CTLCOLOREDIT
			auto style = ::GetWindowLongPtr(hWnd, GWL_STYLE);
			if ((style & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST)
			{
				RECT bkRc = rc;
				bkRc.left += 1;
				bkRc.top += 1;
				bkRc.right = arrowRc.left - 1;
				bkRc.bottom -= 1;
				::FillRect(hdc, &bkRc, PluginDarkMode::getBackgroundBrush()); // erase background on item change

				auto index = static_cast<int>(::SendMessage(hWnd, CB_GETCURSEL, 0, 0));
				if (index != CB_ERR)
				{
					::SetTextColor(hdc, PluginDarkMode::getTextColor());
					::SetBkColor(hdc, PluginDarkMode::getBackgroundColor());
					auto bufferLen = static_cast<size_t>(::SendMessage(hWnd, CB_GETLBTEXTLEN, index, 0));
					TCHAR* buffer = new TCHAR[(bufferLen + 1)];
					::SendMessage(hWnd, CB_GETLBTEXT, index, reinterpret_cast<LPARAM>(buffer));

					RECT textRc = rc;
					textRc.left += 4;
					textRc.right = arrowRc.left - 5;

					::DrawText(hdc, buffer, -1, &textRc, DT_NOPREFIX | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
					delete[]buffer;
				}
			}

			POINT ptCursor = {};
			::GetCursorPos(&ptCursor);
			ScreenToClient(hWnd, &ptCursor);

			bool isHot = PtInRect(&rc, ptCursor);

			::SetTextColor(hdc, isHot ? PluginDarkMode::getTextColor() : PluginDarkMode::getDarkerTextColor());
			::SetBkColor(hdc, isHot ? PluginDarkMode::getHotBackgroundColor() : PluginDarkMode::getBackgroundColor());
			::ExtTextOut(hdc,
				arrowRc.left + (arrowRc.right - arrowRc.left) / 2 - _dpiManager.scaleX(4),
				arrowRc.top + 3,
				ETO_OPAQUE | ETO_CLIPPED,
				&arrowRc, L"˅",
				1,
				nullptr);
			::SetBkColor(hdc, PluginDarkMode::getBackgroundColor());

			POINT edge[] = {
				{arrowRc.left - 1, arrowRc.top},
				{arrowRc.left - 1, arrowRc.bottom}
			};
			::Polyline(hdc, edge, _countof(edge));

			::SelectObject(hdc, holdPen);
			::SelectObject(hdc, holdBrush);

			::EndPaint(hWnd, &ps);
			return 0;
		}

		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, ComboBoxSubclass, uIdSubclass);
			break;
		}

		case WM_CTLCOLOREDIT:
			return PluginDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			return PluginDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));

		case WM_ERASEBKGND:
		{
			if (PluginDarkMode::isEnabled())
			{
				RECT rc = {};
				GetClientRect(hWnd, &rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, PluginDarkMode::getDarkerBackgroundBrush());
				return TRUE;
			}
			break;
		}

		case WM_PRINTCLIENT:
		{
			if (PluginDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassComboBoxControl(HWND hwnd)
	{
		SetWindowSubclass(hwnd, ComboBoxSubclass, g_comboBoxSubclassID, 0);
	}

	void paintGroupbox(HWND hwnd, HDC hdc, ButtonData& buttonData)
	{
		DWORD nStyle = GetWindowLong(hwnd, GWL_STYLE);
		int iPartID = BP_GROUPBOX;
		int iStateID = GBS_NORMAL;

		if (nStyle & WS_DISABLED)
		{
			iStateID = GBS_DISABLED;
		}

		RECT rcClient = {};
		GetClientRect(hwnd, &rcClient);

		RECT rcText = rcClient;
		RECT rcBackground = rcClient;

		HFONT hFont = nullptr;
		HFONT hOldFont = nullptr;
		HFONT hCreatedFont = nullptr;
		LOGFONT lf = {};
		if (SUCCEEDED(GetThemeFont(buttonData.hTheme, hdc, iPartID, iStateID, TMT_FONT, &lf)))
		{
			hCreatedFont = CreateFontIndirect(&lf);
			hFont = hCreatedFont;
		}

		if (!hFont)
		{
			hFont = reinterpret_cast<HFONT>(SendMessage(hwnd, WM_GETFONT, 0, 0));
		}

		hOldFont = static_cast<HFONT>(::SelectObject(hdc, hFont));

		WCHAR szText[256] = { '\0' };
		GetWindowText(hwnd, szText, _countof(szText));

		auto style = static_cast<long>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		bool isCenter = (style & BS_CENTER) == BS_CENTER;

		if (szText[0])
		{
			SIZE textSize = {};
			GetTextExtentPoint32(hdc, szText, static_cast<int>(wcslen(szText)), &textSize);

			int centerPosX = isCenter ? ((rcClient.right - rcClient.left - textSize.cx) / 2) : 7;

			rcBackground.top += textSize.cy / 2;
			rcText.left += centerPosX;
			rcText.bottom = rcText.top + textSize.cy;
			rcText.right = rcText.left + textSize.cx + 4;

			ExcludeClipRect(hdc, rcText.left, rcText.top, rcText.right, rcText.bottom);
		}
		else
		{
			SIZE textSize = {};
			GetTextExtentPoint32(hdc, L"M", 1, &textSize);
			rcBackground.top += textSize.cy / 2;
		}

		RECT rcContent = rcBackground;
		GetThemeBackgroundContentRect(buttonData.hTheme, hdc, BP_GROUPBOX, iStateID, &rcBackground, &rcContent);
		ExcludeClipRect(hdc, rcContent.left, rcContent.top, rcContent.right, rcContent.bottom);

		//DrawThemeParentBackground(hwnd, hdc, &rcClient);
		DrawThemeBackground(buttonData.hTheme, hdc, BP_GROUPBOX, iStateID, &rcBackground, nullptr);

		SelectClipRgn(hdc, nullptr);

		if (szText[0])
		{
			rcText.right -= 2;
			rcText.left += 2;

			DTTOPTS dtto = { sizeof(DTTOPTS), DTT_TEXTCOLOR };
			dtto.crText = PluginDarkMode::getTextColor();

			DWORD textFlags = isCenter ? DT_CENTER : DT_LEFT;

			DrawThemeTextEx(buttonData.hTheme, hdc, BP_GROUPBOX, iStateID, szText, -1, textFlags | DT_SINGLELINE, &rcText, &dtto);
		}

		if (hCreatedFont) DeleteObject(hCreatedFont);
		SelectObject(hdc, hOldFont);
	}

	constexpr UINT_PTR g_groupboxSubclassID = 42;

	LRESULT CALLBACK GroupboxSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		UNREFERENCED_PARAMETER(uIdSubclass);

		auto pButtonData = reinterpret_cast<ButtonData*>(dwRefData);

		switch (uMsg)
		{
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, GroupboxSubclass, g_groupboxSubclassID);
			delete pButtonData;
			break;
		case WM_ERASEBKGND:
			if (PluginDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
			{
				return TRUE;
			}
			else
			{
				break;
			}
		case WM_THEMECHANGED:
			pButtonData->closeTheme();
			break;
		case WM_PRINTCLIENT:
		case WM_PAINT:
			if (PluginDarkMode::isEnabled() && pButtonData->ensureTheme(hWnd))
			{
				PAINTSTRUCT ps = {};
				HDC hdc = reinterpret_cast<HDC>(wParam);
				if (!hdc)
				{
					hdc = BeginPaint(hWnd, &ps);
				}

				paintGroupbox(hWnd, hdc, *pButtonData);

				if (ps.hdc)
				{
					EndPaint(hWnd, &ps);
				}

				return 0;
			}
			else
			{
				break;
			}
			break;
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassGroupboxControl(HWND hwnd)
	{
		DWORD_PTR pButtonData = reinterpret_cast<DWORD_PTR>(new ButtonData());
		SetWindowSubclass(hwnd, GroupboxSubclass, g_groupboxSubclassID, pButtonData);
	}

	constexpr UINT_PTR g_headerSubclassID = 42;
	constexpr UINT_PTR g_listViewSubclassID = 42;

	LRESULT DrawHeaderItem(HeaderItemData& headerItem, HWND hHeader, HDC hdc, const RECT& rcItem, int itemID, LONG_PTR headerStyle,
		const POINT& mousePosition, const HDHITTESTINFO& headerHitTest, const RECT& rcParentClient, int parentScrollPos)
	{
		HTHEME hTheme = headerItem.hTheme;
		SIZE szItem = { rcItem.right - rcItem.left, rcItem.bottom - rcItem.top };

		// Information on Control Style
		bool clickableHeaderStyle = (headerStyle & HDS_BUTTONS) == HDS_BUTTONS;

		// Information on Header Item
		HDITEM hdItem = {};
		TCHAR buffer[MAX_PATH] = { '/0' };
		hdItem.mask = HDI_TEXT | HDI_FORMAT | HDI_BITMAP | HDI_IMAGE;
		hdItem.pszText = buffer;
		hdItem.cchTextMax = static_cast<int>(std::size(buffer));
		Header_GetItem(hHeader, itemID, &hdItem);

		// Temporary modifiable rectangle for item
		RECT txtRC = rcItem;
		RECT dropDownRc;

		// Create a clipping region
		HRGN clipRgn = CreateRectRgnIndirect(&txtRC);
		SelectClipRgn(hdc, clipRgn);
		DeleteObject(clipRgn);

		// Filterbar is somewhat deprecated, but will double width of column
		if (headerStyle & HDS_FILTERBAR)
			txtRC.bottom /= 2;

		// Text format parameters
		DWORD textFormat;
		textFormat = (hdItem.fmt & HDF_LEFT) ? DT_LEFT : 0;
		if (textFormat == 0)
			textFormat = (hdItem.fmt & HDF_CENTER) ? DT_CENTER : 0;
		if (textFormat == 0)
			textFormat = (hdItem.fmt & HDF_RIGHT) ? DT_RIGHT : 0;
		textFormat |= DT_NOPREFIX | DT_WORD_ELLIPSIS | DT_MODIFYSTRING;

		int iTextStateID = HIS_NORMAL;
		DTTOPTS dtto = { sizeof(DTTOPTS), DTT_TEXTCOLOR };
		dtto.crText = PluginDarkMode::getTextColor();

		// Calculate Dropdown box
		if ((hdItem.fmt & HDF_SPLITBUTTON))
		{
			Header_GetItemDropDownRect(hHeader, itemID, &dropDownRc);
			int cx = dropDownRc.right - dropDownRc.left;
			dropDownRc.left = rcItem.right - cx;
			dropDownRc.right = rcItem.right;
			txtRC.right -= cx;

			if (headerStyle & HDS_FILTERBAR)
				dropDownRc.bottom /= 2;
		}

		// Draw hot/pressed item if appliable
		bool vItemPressed = false, vItemFocused = false;
		if (clickableHeaderStyle)
		{
			// Fix hit test bug with mouse position and parent scrolling
			vItemFocused = headerHitTest.iItem == itemID && mousePosition.x >= parentScrollPos &&
				mousePosition.x < rcParentClient.right + parentScrollPos;
			vItemPressed = (vItemFocused && (GetKeyState(VK_LBUTTON) & 0x8000) != 0);

#ifdef _DEBUG_DARK_MODE
			TCHAR debug[128] = {0};
			wsprintf(debug, L"HitTest(%d); MousePosition(%d, %d); Valid Boundaries(%d, %d, %d, %d). TS = %d\n",
				headerHitTest.iItem, mousePosition.x, mousePosition.y, parentScrollPos, rcParentClient.top,
				rcParentClient.right + parentScrollPos, rcParentClient.bottom, (int)GetTickCount64());
			OutputDebugString(debug);
#endif
			if (vItemPressed)
			{
				FillRect(hdc, &txtRC, getHardlightBackgroundBrush());
				iTextStateID = HIS_PRESSED;
			}
			else if (vItemFocused)
				FillRect(hdc, &txtRC, getSoftlightBackgroundBrush());
		}

		// Draw Sort arrow
		if (hdItem.fmt & (HDF_SORTDOWN | HDF_SORTUP))
		{
			int iSortArrowState = (hdItem.fmt & HDF_SORTDOWN) > 0 ? HSAS_SORTEDDOWN : HSAS_SORTEDUP;
			headerItem.drawSortArrow(hdc, iSortArrowState, rcItem);
		}

		SIZE szTxtRect = { txtRC.right - txtRC.left, txtRC.bottom - txtRC.top };

		// Calculate text rectangle output (after processing the combo box)
		RECT rcTxtOutput = {};
		SIZE szTxtOutput = {};
		buffer[MAX_PATH - 1] = '\0';
		if (buffer[0] > 0)
			GetThemeTextExtent(hTheme, hdc, HP_HEADERITEM, iTextStateID, buffer, -1,
				textFormat, ((textFormat & DT_CENTER) > 0) ? &rcItem : &txtRC, &rcTxtOutput);
		szTxtOutput = { rcTxtOutput.right - rcTxtOutput.left, rcTxtOutput.bottom - rcTxtOutput.top };

		// Process item checkbox (current control does nothing on listviews, so we skip here)
		// if (hdItem.fmt & HDF_CHECKBOX)

		// Process item bitmap
		SIZE szBitmap = {};
		bool bitmapValid = false;
		int dxGap = 0;
		int dxTextGap = 0;
		if ((hdItem.fmt & HDF_BITMAP) > 0 || (hdItem.fmt & HDF_IMAGE) > 0)
		{
			RECT rcBitmap = {};
			HBITMAP hBitmap = nullptr;
			HIMAGELIST imageList = nullptr;
			if (hdItem.fmt & HDF_IMAGE)
			{
				int ix, iy;
				imageList = Header_GetImageList(hHeader);
				ImageList_GetIconSize(imageList, &ix, &iy);
				szBitmap = { ix, iy };
				bitmapValid = true;
			}

			BITMAP bp = {};
			if (GetObject(hdItem.hbm, sizeof(bp), &bp))
			{
				szBitmap = { bp.bmWidth, bp.bmHeight };
				hBitmap = hdItem.hbm;
				bitmapValid = true;
			}

			// Create gap between text and image
			if (bitmapValid)
			{
				dxGap = 5;
				dxTextGap = Header_GetBitmapMargin(hHeader);
			}

			// if contains also text, align image relative to text
			if (buffer[0] != 0)
			{
				if ((textFormat & DT_CENTER) > 0)
				{
					rcBitmap.left = rcItem.left + (szItem.cx - szTxtOutput.cx - szBitmap.cx - dxGap - dxTextGap) / 2;
					if (hdItem.fmt & HDF_BITMAP_ON_RIGHT)
						rcBitmap.left += szTxtOutput.cx + dxGap + dxTextGap;
				}
				else
					rcBitmap.left = (hdItem.fmt & HDF_BITMAP_ON_RIGHT) ? txtRC.left + szTxtOutput.cx + (dxGap * 2) : txtRC.left + dxGap;

				rcBitmap.top = (rcItem.bottom - szBitmap.cy) / 2;
				rcBitmap.right = rcBitmap.left + szBitmap.cx;
				rcBitmap.bottom = rcBitmap.top + szBitmap.cy;

				// For both right aligned image and text, override position
				if ((hdItem.fmt & HDF_BITMAP_ON_RIGHT) > 0 && (textFormat & DT_RIGHT) > 0)
				{
					rcBitmap.left = txtRC.right - szBitmap.cx - dxGap;
					rcBitmap.right = rcBitmap.left + szBitmap.cx;
				}
				// Lastly, for right aligned text (left image), position it before text
				else if ((textFormat & DT_RIGHT) > 0)
				{
					rcBitmap.left = txtRC.right - (szBitmap.cx + szTxtOutput.cx + 10);
					rcBitmap.right = rcBitmap.left + szBitmap.cx;
				}
			}
			// Else, align purely without text
			else
			{
				if ((hdItem.fmt & HDF_CENTER) > 0)
					rcBitmap.left = rcItem.left + (szItem.cx - szBitmap.cx) / 2;
				else if ((hdItem.fmt & HDF_RIGHT) > 0)
					rcBitmap.left = txtRC.right - szBitmap.cx;
				else
					rcBitmap.left = dxGap;

				rcBitmap.top = (rcItem.bottom - szBitmap.cy) / 2;
				rcBitmap.right = rcBitmap.left + szBitmap.cx;
				rcBitmap.bottom = rcBitmap.top + szBitmap.cy;
			}

			// Offset pressed item
			if (vItemPressed)
			{
				rcBitmap.left += _dpiManager.scaleX(1); rcBitmap.right += _dpiManager.scaleX(1);
				rcBitmap.top += _dpiManager.scaleY(1); rcBitmap.bottom += _dpiManager.scaleY(1);
			}

			// Draw either one of the images. Priority for ImageList
			if (imageList)
				ImageList_Draw(imageList, hdItem.iImage, hdc, rcBitmap.left, rcBitmap.top, ILD_NORMAL);
			else if (hBitmap)
			{
				HDC memDC = CreateCompatibleDC(NULL);
				HBITMAP hOldBitmap = SelectBitmap(memDC, hBitmap);

				BLENDFUNCTION bf1;
				bf1.BlendOp = AC_SRC_OVER;
				bf1.BlendFlags = 0;
				bf1.SourceConstantAlpha = 0xff;
				bf1.AlphaFormat = AC_SRC_ALPHA;
				GdiAlphaBlend(hdc, rcBitmap.left, rcBitmap.top, szBitmap.cx, szBitmap.cy, memDC, 0, 0, szBitmap.cx, szBitmap.cy, bf1);
				SelectBitmap(memDC, hOldBitmap);
				DeleteDC(memDC);
			}
		}

		// Process caption text.
		constexpr const int margins = 10;
		if (buffer[0] != 0)
		{
			if (szTxtOutput.cx > szTxtRect.cx)
			{
				rcTxtOutput.left = txtRC.left;
				rcTxtOutput.right = txtRC.right;
				szTxtOutput = { rcTxtOutput.right - rcTxtOutput.left, rcTxtOutput.bottom - rcTxtOutput.top };
			}

			// Vertical align
			rcTxtOutput.top = (txtRC.bottom - szTxtOutput.cy) / 2;
			rcTxtOutput.bottom = rcTxtOutput.top + szTxtOutput.cy;

			// Proper align output rectangle inside client box according to text alignment and previous image (if exist)
			if ((textFormat & DT_RIGHT) > 0)
			{
				rcTxtOutput.right = txtRC.right - 5;
				rcTxtOutput.left = rcTxtOutput.right - szTxtOutput.cx;

				if (bitmapValid && ((hdItem.fmt & HDF_BITMAP_ON_RIGHT) > 0))
				{
					rcTxtOutput.left -= (szBitmap.cx + dxGap + dxTextGap);
					rcTxtOutput.right -= (szBitmap.cx + dxGap + dxTextGap);
				}
			}
			else if ((textFormat & DT_CENTER) > 0)
			{
				rcTxtOutput.left = rcItem.left + (szItem.cx - szTxtOutput.cx - szBitmap.cx - dxGap - dxTextGap) / 2;
				rcTxtOutput.right = rcTxtOutput.left + szTxtOutput.cx;

				if (bitmapValid && ((hdItem.fmt & HDF_BITMAP_ON_RIGHT) == 0))
				{
					rcTxtOutput.left += szBitmap.cx + dxGap + dxTextGap;
					rcTxtOutput.right += szBitmap.cx + dxGap + dxTextGap;
				}
			}
			else
			{
				rcTxtOutput.left += 5 + (((hdItem.fmt & HDF_BITMAP_ON_RIGHT) > 0) ? 0 : szBitmap.cx + dxGap + dxTextGap);
				rcTxtOutput.right += 5 + (((hdItem.fmt & HDF_BITMAP_ON_RIGHT) > 0) ? 0 : szBitmap.cx + dxGap + dxTextGap);
			}

			// Offset text box for pressed items
			if (vItemPressed)
			{
				rcTxtOutput.left += _dpiManager.scaleX(1); rcTxtOutput.right += _dpiManager.scaleX(1);
				rcTxtOutput.top += _dpiManager.scaleY(1); rcTxtOutput.bottom += _dpiManager.scaleY(1);
			}

			// Sanity check
			if (rcTxtOutput.right > txtRC.right - dxGap)
				rcTxtOutput.right = txtRC.right - dxGap;
			if (rcTxtOutput.left > rcTxtOutput.right)
				rcTxtOutput.left = rcTxtOutput.right;

			// Draw text
			DrawThemeTextEx(hTheme, hdc, HP_HEADERITEM, iTextStateID, buffer, -1, textFormat, &rcTxtOutput, &dtto);
		}

		// Draw separator line
		auto hOldPen = static_cast<HPEN>(::SelectObject(hdc, PluginDarkMode::getEdgePen()));
		POINT edges[] = {
			{rcItem.right - _dpiManager.scaleX(1), rcItem.top},
			{rcItem.right - _dpiManager.scaleX(1), rcItem.bottom}
		};
		Polyline(hdc, edges, _countof(edges));
		::SelectObject(hdc, hOldPen);

		// Draw Combo box if focused
		if ((hdItem.fmt & HDF_SPLITBUTTON) && (vItemPressed || vItemFocused))
		{
			if (vItemPressed)
				FillRect(hdc, &dropDownRc, getHardlightBackgroundBrush());
			else
				FillRect(hdc, &dropDownRc, getSoftlightBackgroundBrush());

			// Combo box part is not present, simulate with down arrow and an edge
			headerItem.lastDropDownTrackWasHot = PtInRect(&dropDownRc, mousePosition);
			headerItem.drawComboArrow(hdc, dropDownRc, headerItem.lastDropDownTrackWasHot);
			::SelectObject(hdc, PluginDarkMode::getEdgePen());
			POINT edges[] = {
				{dropDownRc.left, rcItem.top},
				{dropDownRc.left, rcItem.bottom}
			};
			Polyline(hdc, edges, _countof(edges));
		}

		// TODO: handle HDS_FILTERBAR style? How?
		// https://devblogs.microsoft.com/oldnewthing/20120227-00/?p=8223

		return CDRF_SKIPDEFAULT;
	}

	LRESULT DrawHeaderOverflow(HWND hHeader, HDC hdc, RECT& rcClient)
	{
		RECT overflowRect = {};
		Header_GetOverflowRect(hHeader, &overflowRect);

		if (overflowRect.left == overflowRect.right)
			return S_OK;

		POINT cursorPos;
		HDHITTESTINFO hti;
		GetCursorPos(&cursorPos);
		ScreenToClient(hHeader, &cursorPos);
		hti.pt = cursorPos;
		SendMessage(hHeader, HDM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti));

		// Currently, dark mode overflow style is somewhat buggy, don't see a way to properly fix.
		// So we only draw the overflow if the user hits the box.
		if ((hti.flags & HHT_ONOVERFLOW) == 0)
			return S_OK;

		SelectClipRgn(hdc, NULL);

		DRAWTEXTPARAMS dt = {};
		RECT rcOutText = overflowRect;
		dt.cbSize = sizeof(dt);

		HFONT hFont = ::CreateFont(_dpiManager.scaleX(18), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
			OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Arial"));
		HFONT hOldFont = SelectFont(hdc, hFont);

		DrawTextEx(hdc, reinterpret_cast<LPWSTR>(std::wstring(TEXT("»")).data()), -1, &rcOutText,
			DT_RIGHT | DT_VCENTER | DT_CALCRECT, &dt);

		SIZE szTextOut = { rcOutText.right - rcOutText.left, rcOutText.bottom };
		rcOutText.top = (rcClient.bottom - rcOutText.bottom) / 2;
		rcOutText.bottom = rcOutText.top + szTextOut.cy;
		rcOutText.right = overflowRect.right;
		rcOutText.left = rcOutText.right - szTextOut.cx;

		if (hti.flags & HHT_ONOVERFLOW)
		{
			SelectPen(hdc, getLightEdgePen());
			SelectBrush(hdc, getSoftlightBackgroundBrush());
			RoundRect(hdc, rcOutText.left, rcOutText.top, rcOutText.right, rcOutText.bottom, 3, 3);
		}

		rcOutText.top -= 1;
		SetTextColor(hdc, getTextColor());
		SetBkMode(hdc, TRANSPARENT);
		DrawTextEx(hdc, reinterpret_cast<LPWSTR>(std::wstring(TEXT("»")).data()), -1, &rcOutText,
			DT_RIGHT | DT_VCENTER, &dt);
		SelectFont(hdc, hOldFont);
		DeleteObject(hFont);

		return S_OK;
	}

	void HotTrackHeaderComboBox(HeaderItemData& headerItem, HWND hHeader, DWORD cursorParams)
	{
		// Mouse coords
		POINT cursorPos = { LOWORD(cursorParams), HIWORD(cursorParams) };
		HDHITTESTINFO hti;

		// Control style
		LONG_PTR headerStyle = GetWindowLongPtr(hHeader, GWL_STYLE);
		bool clickableHeaderStyle = (headerStyle & HDS_BUTTONS) > 0;

		// Not clickable, bye.
		if (!clickableHeaderStyle)
			return;

		// Get hit item
		hti.pt = cursorPos;
		SendMessage(hHeader, HDM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti));
		HDITEM hdItem;
		hdItem.mask = HDI_FORMAT;
		Header_GetItem(hHeader, hti.iItem, &hdItem);

		// Don't have dropdown box, bye.
		if ((hdItem.fmt & HDF_SPLITBUTTON) == 0)
			return;

		// Get the real coordinates of rectangle (Header_GetItemDropDownRect is broken for some cases, we only use the width)
		RECT rcItem, dropRc;
		SIZE szDropDown;
		Header_GetItemRect(hHeader, hti.iItem, &rcItem);
		Header_GetItemDropDownRect(hHeader, hti.iItem, &dropRc);
		szDropDown.cx = dropRc.right - dropRc.left;
		SetRect(&dropRc, rcItem.right - szDropDown.cx, dropRc.top, rcItem.right, dropRc.bottom);

		if ((PtInRect(&dropRc, cursorPos) && headerItem.lastDropDownTrackWasHot == false)
			|| (!PtInRect(&dropRc, cursorPos) && headerItem.lastDropDownTrackWasHot))
		{
			// Redraw only this item
			HDC hdc = GetDC(hHeader);
			HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hHeader, WM_GETFONT, 0, 0)); // must set the DC font first
			HFONT hOldFont = SelectFont(hdc, hFont);
			RECT clientRect, parentRect;
			GetClientRect(hHeader, &clientRect);
			GetClientRect(GetParent(hHeader), &parentRect);
			SCROLLINFO si = {};
			si.cbSize = sizeof(si);
			si.fMask = SIF_POS;
			GetScrollInfo(GetParent(hHeader), SB_HORZ, &si);

			DrawHeaderItem(headerItem, hHeader, hdc, rcItem, hti.iItem, headerStyle, cursorPos, hti, parentRect, si.nPos);
			RedrawWindow(hHeader, &rcItem, NULL, RDW_VALIDATE);
			SelectFont(hdc, hOldFont);
			ReleaseDC(hHeader, hdc);
		}
	}

	LRESULT CALLBACK HeaderSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR /*uIdSubclass*/,
		DWORD_PTR dwRefData
	)
	{
		auto pHeaderItem = reinterpret_cast<HeaderItemData*>(dwRefData);

		switch (uMsg)
		{
		case WM_PAINT:
		{
			//break;
			pHeaderItem->ensureTheme();

				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				// Erase control background
				RECT rcClient, rcParentClient;
				GetClientRect(hWnd, &rcClient);
				GetClientRect(GetParent(hWnd), &rcParentClient);
				FillRect(hdc, &rcClient, getDarkerBackgroundBrush());
				LONG_PTR headerStyle = GetWindowLongPtr(hWnd, GWL_STYLE);

				// Extra informations for item drawing
				POINT mousePos;
				GetCursorPos(&mousePos);
				ScreenToClient(hWnd, &mousePos);
				HDHITTESTINFO hti;
				hti.pt = mousePos;
				SendMessage(hWnd, HDM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti));
				SCROLLINFO si = {};
				si.cbSize = sizeof(si);
				si.fMask = SIF_POS;
				GetScrollInfo(GetParent(hWnd), SB_HORZ, &si);

				// Change font
				HFONT lstFont = reinterpret_cast<HFONT>(::SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0));
				HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, lstFont));

			// Draw items
			int count = static_cast<int>(Header_GetItemCount(hWnd));
			RECT wRc = {};
			for (int i = 0; i < count; i++)
			{
				Header_GetItemRect(hWnd, i, &wRc);
				DrawHeaderItem(*pHeaderItem, hWnd, hdc, wRc, i, headerStyle, mousePos, hti, rcParentClient, si.nPos);
			}

			// Draw header overflow if appliable
			if ((headerStyle & HDS_OVERFLOW) > 0)
				DrawHeaderOverflow(hWnd, hdc, rcClient);

				//Cleanup
				SelectObject(hdc, oldFont);
				EndPaint(hWnd, &ps);

				return TRUE;
			}

			case WM_THEMECHANGED:
			{
				pHeaderItem->closeTheme();
				break;
			}

			case WM_NCDESTROY:
			{
				RemoveWindowSubclass(hWnd, HeaderSubclass, g_headerSubclassID);
				delete pHeaderItem;
				break;
			}

			case WM_SIZE:
			{
				pHeaderItem->reloadStructBitmaps();
				break;
			}

			case WM_MOUSEMOVE:
			{
				HotTrackHeaderComboBox(*pHeaderItem, hWnd, static_cast<DWORD>(lParam));
				break;
			}
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassHeaderControl(HWND hwndHeader)
	{
		SetWindowSubclass(hwndHeader, HeaderSubclass, g_headerSubclassID, reinterpret_cast<DWORD_PTR>(new HeaderItemData()));
	}

	LRESULT CALLBACK ListViewSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR /*uIdSubclass*/,
		DWORD_PTR dwRefData
	)
	{
		switch (uMsg)
		{
		case WM_THEMECHANGED:
		{
			HWND hHeader = ListView_GetHeader(hWnd);

			AllowDarkModeForWindow(hWnd, PluginDarkMode::isEnabled());
			AllowDarkModeForWindow(hHeader, PluginDarkMode::isEnabled());

			HTHEME hTheme = OpenThemeData(nullptr, L"ItemsView");
			if (hTheme)
			{
				COLORREF color;
				if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color)))
				{
					ListView_SetTextColor(hWnd, PluginDarkMode::isEnabled() ? PluginDarkMode::getTextColor() : color);
				}

				if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_FILLCOLOR, &color)))
				{
					ListView_SetTextBkColor(hWnd, PluginDarkMode::isEnabled() ? PluginDarkMode::getSofterBackgroundColor() : color);
					ListView_SetBkColor(hWnd, PluginDarkMode::isEnabled() ? PluginDarkMode::getSofterBackgroundColor() : color);
				}

				CloseThemeData(hTheme);
			}

			SendMessage(hHeader, WM_THEMECHANGED, wParam, lParam);
			RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE);
			break;
		}

		case WM_NCDESTROY:
		{
			RemoveWindowSubclass(hWnd, ListViewSubclass, g_listViewSubclassID);
			break;
		}
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassListViewControl(HWND hwndListView)
	{
		SetWindowSubclass(hwndListView, ListViewSubclass, g_listViewSubclassID, 0);
	}

	constexpr UINT_PTR g_spinControlSubclassID = 42;

	void renderArrow(HDC hdc, const RECT& rcArrow, SpinArrowDirection direction, bool bPressed, LONG_PTR nStyle)
	{
		struct FPOINT {
			float x = 0.0f;
			float y = 0.0f;
		};

		FPOINT ptTriangleF[3] = {};
		POINT ptTriangle[3] = {};

		switch (direction)
		{
		case SpinArrowDirection::Left:
			ptTriangleF[0] = { 1.0f, 0.0f };  
			ptTriangleF[1] = { 0.0f, 0.5f };
			ptTriangleF[2] = { 1.0f, 1.0f };
			break;
		case SpinArrowDirection::Right:
			ptTriangleF[0] = { 0.0f, 0.0f };
			ptTriangleF[1] = { 1.0f, 0.5f };
			ptTriangleF[2] = { 0.0f, 1.0f };
			break;
		case SpinArrowDirection::Up:
			ptTriangleF[0] = { 0.0f, 1.0f };
			ptTriangleF[1] = { 0.5f, 0.0f };
			ptTriangleF[2] = { 1.0f, 1.0f };
			break;
		case SpinArrowDirection::Down:
			ptTriangleF[0] = { 0.0f, 0.0f };
			ptTriangleF[1] = { 0.5f, 1.0f };
			ptTriangleF[2] = { 1.0f, 0.0f };
			break;
		}

		SIZE szArrowRc = { rcArrow.right - rcArrow.left, rcArrow.bottom - rcArrow.top };
		SIZE szTriangleRect = { _dpiManager.scaleX(2), _dpiManager.scaleY(2) };
		for (int i = 0; i < 3; i++)
		{
			ptTriangle[i].x = ptTriangleF[i].x * szTriangleRect.cx; // Scale triangle
			ptTriangle[i].y = ptTriangleF[i].y * szTriangleRect.cy;
			ptTriangle[i].x += rcArrow.left + ((szArrowRc.cx - ptTriangle[i].x) / 2);    // Centralize
			ptTriangle[i].y += rcArrow.top + ((szArrowRc.cy - ptTriangle[i].y) / 2);
		}

		SelectObject(hdc, (nStyle & WS_DISABLED) > 0 ? getLightEdgePen() : getEdgePen());
		HBRUSH hTriangleBrush = (nStyle & WS_DISABLED) > 0 ? getDarkerTextBrush() : getTextBrush();
		HBRUSH hOldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, hTriangleBrush));

		Polygon(hdc, &ptTriangle[0], 3);
		SelectObject(hdc, hOldBrush);
	}

	void renderSpinButton(const SpinData& pSpinData, HDC hdc, const RECT& rcArrowOne, const RECT& rcArrowTwo, int iStateID1, int iStateID2)
	{
		BLENDFUNCTION bf1;
		bf1.BlendOp = AC_SRC_OVER;
		bf1.BlendFlags = 0;
		bf1.SourceConstantAlpha = 0xff;
		bf1.AlphaFormat = AC_SRC_ALPHA;

		SIZE rcSizes[2] = {
			{ rcArrowOne.right - rcArrowOne.left, rcArrowOne.bottom - rcArrowOne.top },
			{ rcArrowTwo.right - rcArrowTwo.left, rcArrowTwo.bottom - rcArrowTwo.top },
		};

		HDC memDC = CreateCompatibleDC(hdc);

		HBITMAP hOldBitmap = reinterpret_cast<HBITMAP>(SelectObject(memDC, pSpinData.SpinArrowOne[iStateID1 - 1]));
		BitBlt(hdc, rcArrowOne.left, rcArrowOne.top, rcSizes[0].cx, rcSizes[0].cy, memDC, 0, 0, SRCCOPY);
		SelectObject(memDC, pSpinData.SpinArrowTwo[iStateID2 - 1]);
		BitBlt(hdc, rcArrowTwo.left, rcArrowTwo.top, rcSizes[1].cx, rcSizes[1].cy, memDC, 0, 0, SRCCOPY);

		SelectObject(memDC, hOldBitmap);
		DeleteDC(memDC);
	}

	void paintUpDownButtons(HWND hWnd, HDC hdc, SpinData& pSpinData)
	{
		RECT rcClient{};
		::GetClientRect(hWnd, &rcClient);

		// Determines up/down or left/right
		RECT rcArrowOne;
		RECT rcArrowTwo;
		SpinButtonType bSpinDirection = SpinButtonType::LeftRight;

		if (rcClient.right - rcClient.left > rcClient.bottom - rcClient.top)
		{
			rcArrowOne = {
				rcClient.left, rcClient.top,
				rcClient.right - ((rcClient.right - rcClient.left) / 2) , rcClient.bottom
			};
			rcArrowTwo = {
				rcArrowOne.right, rcClient.top,
				rcClient.right, rcClient.bottom
			};
		}
		else
		{
			bSpinDirection = SpinButtonType::UpDown;
			rcArrowOne = {
				rcClient.left, rcClient.top,
				rcClient.right, rcClient.bottom - ((rcClient.bottom - rcClient.top) / 2)
			};
			rcArrowTwo = {
				rcClient.left, rcArrowOne.bottom,
				rcClient.right, rcClient.bottom
			};
		}

		POINT ptCursor = {};
		::GetCursorPos(&ptCursor);
		::ScreenToClient(hWnd, &ptCursor);

		bool isHotOne = ::PtInRect(&rcArrowOne, ptCursor);
		bool isHotTwo = ::PtInRect(&rcArrowTwo, ptCursor);
		bool bPressed = (GetKeyState(VK_LBUTTON) & 0x8000) != 0;
		LONG_PTR nStyle = GetWindowLongPtr(hWnd, GWL_STYLE); // To check enabled / disabled

		int iStateID1 = (bPressed && isHotOne) ? UPS_PRESSED : (isHotOne ? UPS_HOT : UPS_NORMAL);
		int iStateID2 = (bPressed && isHotTwo) ? UPS_PRESSED : (isHotTwo ? UPS_HOT : UPS_NORMAL);
		if (nStyle & WS_DISABLED)
		{
			iStateID1 = UPS_DISABLED;
			iStateID2 = UPS_DISABLED;
		}

		if (BufferedPaintRenderAnimation(hWnd, hdc))
		{
			return;
		}

		BP_ANIMATIONPARAMS animParams = { sizeof(animParams) };
		animParams.style = BPAS_LINEAR;
		if (iStateID1 != pSpinData.iStateID1)
			GetThemeTransitionDuration(pSpinData.hTheme, SPNP_UP, pSpinData.iStateID1, iStateID1, TMT_TRANSITIONDURATIONS, &animParams.dwDuration);
		if (iStateID2 != pSpinData.iStateID2)
			GetThemeTransitionDuration(pSpinData.hTheme, SPNP_UP, pSpinData.iStateID2, iStateID2, TMT_TRANSITIONDURATIONS, &animParams.dwDuration);

		HDC hdcFrom = nullptr;
		HDC hdcTo = nullptr;
		HANIMATIONBUFFER hbpAnimation = nullptr; // BeginBufferedAnimation(hWnd, hdc, &rcClient, BPBF_COMPATIBLEBITMAP, nullptr, &animParams, &hdcFrom, &hdcTo);
		if (hbpAnimation)
		{
			if (hdcFrom)
			{
				renderSpinButton(pSpinData, hdcFrom, rcArrowOne, rcArrowTwo, pSpinData.iStateID1, pSpinData.iStateID2);
			}
			if (hdcTo)
			{
				renderSpinButton(pSpinData, hdcTo, rcArrowOne, rcArrowTwo, iStateID1, iStateID2);
			}

			pSpinData.iStateID1 = iStateID1;
			pSpinData.iStateID2 = iStateID2;

			EndBufferedAnimation(hbpAnimation, TRUE);
		}
		else
		{
			renderSpinButton(pSpinData, hdc, rcArrowOne, rcArrowTwo, iStateID1, iStateID2);
			pSpinData.iStateID1 = iStateID1;
			pSpinData.iStateID2 = iStateID2;
		}
	}

	LRESULT CALLBACK SpinControlSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		auto pSpinData = reinterpret_cast<SpinData*>(dwRefData);

		switch (uMsg)
		{
		case WM_PRINTCLIENT:
		case WM_PAINT:
		{
			if (!PluginDarkMode::isEnabled())
			{
				break;
			}

			if (!pSpinData->ensureTheme(hWnd))
			{
				break;
			}

			PAINTSTRUCT ps{};
			auto hdc = ::BeginPaint(hWnd, &ps);
			paintUpDownButtons(hWnd, hdc, *pSpinData);
			::EndPaint(hWnd, &ps);

			return FALSE;
		}

		case WM_THEMECHANGED:
		{
			pSpinData->closeTheme();
			break;
		}

		case WM_NCDESTROY:
		{
			::RemoveWindowSubclass(hWnd, SpinControlSubclass, uIdSubclass);
			delete pSpinData;
			break;
		}

		case WM_ERASEBKGND:
		{
			if (PluginDarkMode::isEnabled())
			{
				RECT rcClient{};
				::GetClientRect(hWnd, &rcClient);
				::FillRect(reinterpret_cast<HDC>(wParam), &rcClient, PluginDarkMode::getDarkerBackgroundBrush());
				return TRUE;
			}
			break;
		}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassSpinControl(HWND hwnd)
	{
		auto pSpinData = reinterpret_cast<DWORD_PTR>(new SpinData());
		SetWindowSubclass(hwnd, SpinControlSubclass, g_spinControlSubclassID, pSpinData);
	}

	constexpr UINT_PTR g_tabSubclassID = 42;

	LRESULT CALLBACK TabSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		UNREFERENCED_PARAMETER(uIdSubclass);
		UNREFERENCED_PARAMETER(dwRefData);

		switch (uMsg)
		{
		case WM_PAINT:
		{
			if (!PluginDarkMode::isEnabled())
			{
				break;
			}

			LONG_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
			if ((dwStyle & TCS_BUTTONS) || (dwStyle & TCS_VERTICAL))
			{
				break;
			}

			PAINTSTRUCT ps;
			HDC hdc = ::BeginPaint(hWnd, &ps);
			::FillRect(hdc, &ps.rcPaint, PluginDarkMode::getDarkerBackgroundBrush());

			RECT rcClient;
			CopyRect(&rcClient, &ps.rcPaint);
			TabCtrl_AdjustRect(hWnd, false, &rcClient);
			FrameRect(hdc, &rcClient, PluginDarkMode::getSofterBackgroundBrush());

			auto holdPen = static_cast<HPEN>(::SelectObject(hdc, PluginDarkMode::getEdgePen()));

			HRGN holdClip = CreateRectRgn(0, 0, 0, 0);
			if (1 != GetClipRgn(hdc, holdClip))
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			HFONT hFont = reinterpret_cast<HFONT>(SendMessage(hWnd, WM_GETFONT, 0, 0));
			auto hOldFont = SelectObject(hdc, hFont);

			POINT ptCursor = {};
			::GetCursorPos(&ptCursor);
			ScreenToClient(hWnd, &ptCursor);

			int nTabs = TabCtrl_GetItemCount(hWnd);

			int nSelTab = TabCtrl_GetCurSel(hWnd);
			for (int i = 0; i < nTabs; ++i)
			{
				RECT rcItem = {};
				TabCtrl_GetItemRect(hWnd, i, &rcItem);

				RECT rcIntersect = {};
				if (IntersectRect(&rcIntersect, &ps.rcPaint, &rcItem))
				{
					bool bHot = PtInRect(&rcItem, ptCursor);

					POINT edges[] = {
						{rcItem.right - 1, rcItem.top},
						{rcItem.right - 1, rcItem.bottom}
					};
					Polyline(hdc, edges, _countof(edges));
					rcItem.right -= 1;

					HRGN hClip = CreateRectRgnIndirect(&rcItem);

					SelectClipRgn(hdc, hClip);

					SetTextColor(hdc, (bHot || (i == nSelTab) ) ? PluginDarkMode::getTextColor() : PluginDarkMode::getDarkerTextColor());

					// for consistency getBackgroundBrush() 
					// would be better, than getSofterBackgroundBrush(),
					// however default getBackgroundBrush() has same color
					// as getDarkerBackgroundBrush()
					::FillRect(hdc, &rcItem, (i == nSelTab) ? PluginDarkMode::getDarkerBackgroundBrush() : PluginDarkMode::getSofterBackgroundBrush());

					SetBkMode(hdc, TRANSPARENT);

					TCHAR label[MAX_PATH];
					TCITEM tci = {};
					tci.mask = TCIF_TEXT;
					tci.pszText = label;
					tci.cchTextMax = MAX_PATH - 1;

					::SendMessage(hWnd, TCM_GETITEM, i, reinterpret_cast<LPARAM>(&tci));

					RECT rcText = rcItem;
					rcText.left += _dpiManager.scaleX(6);
					rcText.right -= _dpiManager.scaleX(3);

					if (i == nSelTab)
					{
						rcText.bottom -= _dpiManager.scaleX(4);
					}

					DrawText(hdc, label, -1, &rcText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

					DeleteObject(hClip);

					SelectClipRgn(hdc, holdClip);
				}
			}

			SelectObject(hdc, hOldFont);

			SelectClipRgn(hdc, holdClip);
			if (holdClip)
			{
				DeleteObject(holdClip);
				holdClip = nullptr;
			}

			SelectObject(hdc, holdPen);

			EndPaint(hWnd, &ps);
			return 0;
		}
		case WM_NCDESTROY:
			RemoveWindowSubclass(hWnd, TabSubclass, g_tabSubclassID);
			break;
		case WM_CTLCOLOREDIT:
			return PluginDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
			return PluginDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		case WM_ERASEBKGND:
		{
			if (PluginDarkMode::isEnabled())
			{
				RECT rc = {};
				GetClientRect(hWnd, &rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, PluginDarkMode::getDarkerBackgroundBrush());
				return TRUE;
			}
			break;
		}
		case WM_PRINTCLIENT:
		{
			if (PluginDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassTabControl(HWND hwnd)
	{
		SetWindowSubclass(hwnd, TabSubclass, g_tabSubclassID, 0);
	}

	constexpr UINT_PTR g_windowSubclassID = 42;

	LRESULT CALLBACK WindowSubclass(
		HWND hWnd,
		UINT uMsg,
		WPARAM wParam,
		LPARAM lParam,
		UINT_PTR uIdSubclass,
		DWORD_PTR dwRefData
	)
	{
		static bool isItemChecked = false, isSep = false;

		switch (uMsg)
		{
		case WM_CTLCOLOREDIT:
			return PluginDarkMode::onCtlColorSofter(reinterpret_cast<HDC>(wParam));
		case WM_CTLCOLORLISTBOX:
		case WM_CTLCOLORDLG:
		case WM_CTLCOLORSTATIC:
		{
			if (!PluginDarkMode::isEnabled())
				break;

			const size_t classNameLen = 16;
			TCHAR className[classNameLen] = { '\0' };
			GetClassName(reinterpret_cast<HWND>(lParam), className, classNameLen);

			if (wcscmp(className, WC_LINK) == 0)
			{
				HDC hdc = reinterpret_cast<HDC>(wParam);
				::SetTextColor(hdc, PluginDarkMode::getLinkTextColor());
				::SetBkColor(hdc, PluginDarkMode::getDarkerBackgroundColor());
				return reinterpret_cast<LRESULT>(PluginDarkMode::getDarkerBackgroundBrush());
			}

			return PluginDarkMode::onCtlColorDarker(reinterpret_cast<HDC>(wParam));
		}
		case WM_ERASEBKGND:
		{
			if (PluginDarkMode::isEnabled())
			{
				RECT rc = {};
				GetClientRect(hWnd, &rc);
				::FillRect(reinterpret_cast<HDC>(wParam), &rc, PluginDarkMode::getDarkerBackgroundBrush());
				return TRUE;
			}
			break;
		}
		case WM_PRINTCLIENT:
		{
			if (PluginDarkMode::isEnabled())
			{
				return TRUE;
			}
			break;
		}

		case WM_NOTIFY:
		{
			LPNMTBCUSTOMDRAW lpcd = reinterpret_cast<LPNMTBCUSTOMDRAW>(lParam);

			const size_t classNameLen = 16;
			TCHAR className[classNameLen] = { '\0' };
			GetClassName(lpcd->nmcd.hdr.hwndFrom, className, classNameLen);

			// Handle Toolbar Custom Draw
			if (reinterpret_cast<LPNMHDR>(lParam)->code == NM_CUSTOMDRAW && wcscmp(className, TOOLBARCLASSNAME) == 0)
			{
				switch (lpcd->nmcd.dwDrawStage)
				{
				case CDDS_PREPAINT:
				{
					HWND Parent = GetParent(lpcd->nmcd.hdr.hwndFrom);
					if (!PluginDarkMode::isEnabled())
					{
						SetWindowLongPtr(Parent, DWLP_MSGRESULT, CDRF_DODEFAULT);
						return CDRF_DODEFAULT;
					}

					::FillRect(lpcd->nmcd.hdc, &lpcd->nmcd.rc, PluginDarkMode::getDarkerBackgroundBrush());
					SetWindowLongPtr(Parent, DWLP_MSGRESULT, CDRF_NOTIFYITEMDRAW);
					return CDRF_NOTIFYITEMDRAW;
				}
				case CDDS_ITEMPREPAINT:
				{
					// Colors to be returned
					lpcd->clrText = PluginDarkMode::getTextColor();
					lpcd->clrBtnFace = PluginDarkMode::getBackgroundColor();
					lpcd->clrBtnHighlight = PluginDarkMode::getDarkerBackgroundColor();
					lpcd->clrHighlightHotTrack = PluginDarkMode::getHotBackgroundColor();
					lpcd->clrMark = 0;
					lpcd->clrTextHighlight = 0;
					lpcd->nHLStringBkMode = TRANSPARENT;

					HWND Parent = GetParent(lpcd->nmcd.hdr.hwndFrom);

					// Draw button background (if it is not a separator and for checked/hot items)
					TBBUTTONINFO btInfo = {};
					btInfo.cbSize = sizeof(TBBUTTONINFO);
					btInfo.dwMask = TBIF_STYLE;
					SendMessage(lpcd->nmcd.hdr.hwndFrom, TB_GETBUTTONINFO, lpcd->nmcd.dwItemSpec, reinterpret_cast<LPARAM>(&btInfo));

					isItemChecked = ((lpcd->nmcd.uItemState & (CDIS_CHECKED)) != 0);
					isSep = (btInfo.fsStyle & BTNS_SEP) == BTNS_SEP;

					if (isItemChecked && !isSep)
					{
						SelectObject(lpcd->nmcd.hdc, GetStockObject(NULL_PEN));
						SelectObject(lpcd->nmcd.hdc, PluginDarkMode::getSofterBackgroundBrush());
						RoundRect(lpcd->nmcd.hdc, lpcd->nmcd.rc.left, lpcd->nmcd.rc.top, lpcd->nmcd.rc.right, lpcd->nmcd.rc.bottom,
							_dpiManager.scaleX(5), _dpiManager.scaleY(5));
					}

					// Simulate item uncheck so we don't get the blue highlight on a dark themed toolbar
					lpcd->nmcd.uItemState &= ~CDIS_CHECKED;

					DWORD result = TBCDRF_USECDCOLORS | TBCDRF_HILITEHOTTRACK | CDRF_NOTIFYPOSTPAINT;
					SetWindowLongPtr(Parent, DWLP_MSGRESULT, result);
					return result;
				}

				case CDDS_ITEMPOSTPAINT:
				{
					// Post-draws the rectangle border to checked buttons
					if (isItemChecked && !isSep)
					{
						SelectObject(lpcd->nmcd.hdc, PluginDarkMode::getEdgePen());
						SelectObject(lpcd->nmcd.hdc, GetStockObject(HOLLOW_BRUSH));
						RoundRect(lpcd->nmcd.hdc, lpcd->nmcd.rc.left, lpcd->nmcd.rc.top, lpcd->nmcd.rc.right, lpcd->nmcd.rc.bottom,
							_dpiManager.scaleX(5), _dpiManager.scaleY(5));
					}
					break;
				}
				}
			}

			break;
		}
		}
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassWindow(HWND hwnd)
	{
		SetWindowSubclass(hwnd, WindowSubclass, g_windowSubclassID, 0);
	}

	void autoSetupWindowAndChildren(HWND hwndWindow)
	{
		if (PluginDarkMode::isEnabled())
			subclassWindow(hwndWindow);
		else
			RemoveWindowSubclass(hwndWindow, WindowSubclass, g_windowSubclassID);

		setDarkTitleBar(hwndWindow);
		autoSubclassAndThemeChildControls(hwndWindow, true, true);
	}

	void autoThemeChildControls(HWND hwndParent)
	{
		autoSubclassAndThemeChildControls(hwndParent, false, true);
	}

	void autoSubclassAndThemeChildControls(HWND hwndParent, bool subclass, bool theme)
	{
		struct Params
		{
			const wchar_t* themeClassName = nullptr;
			bool subclass = false;
			bool theme = false;
		};

		Params p{
			PluginDarkMode::isEnabled() ? L"DarkMode_Explorer" : nullptr
			, subclass
			, theme
		};

		::EnableThemeDialogTexture(hwndParent, theme && !PluginDarkMode::isEnabled() ? ETDT_ENABLE : ETDT_DISABLE);

		EnumChildWindows(hwndParent, [](HWND hwnd, LPARAM lParam) WINAPI_LAMBDA {
			auto& p = *reinterpret_cast<Params*>(lParam);
			const size_t classNameLen = 16;
			TCHAR className[classNameLen] = { '\0' };
			GetClassName(hwnd, className, classNameLen);

			if (wcscmp(className, WC_BUTTON) == 0)
			{
				auto nButtonStyle = ::GetWindowLongPtr(hwnd, GWL_STYLE) & 0xF;
				switch (nButtonStyle)
				{
				case BS_PUSHBUTTON:
				case BS_DEFPUSHBUTTON:
				case BS_CHECKBOX:
				case BS_AUTOCHECKBOX:
				case BS_RADIOBUTTON:
				case BS_AUTORADIOBUTTON:
				{
					if (p.theme && (nButtonStyle == BS_PUSHBUTTON || nButtonStyle == BS_DEFPUSHBUTTON))
						SetWindowTheme(hwnd, p.themeClassName, nullptr);

					if (p.subclass && PluginDarkMode::isEnabled())
						PluginDarkMode::subclassButtonControl(hwnd);
					if (p.subclass && !PluginDarkMode::isEnabled())
						RemoveWindowSubclass(hwnd, ButtonSubclass, g_buttonSubclassID);
					break;
				}
				case BS_GROUPBOX:
					if (p.subclass && PluginDarkMode::isEnabled())
						PluginDarkMode::subclassGroupboxControl(hwnd);
					if (p.subclass && !PluginDarkMode::isEnabled())
						RemoveWindowSubclass(hwnd, GroupboxSubclass, g_groupboxSubclassID);
					break;
				}
				return TRUE;
			}

			if (wcscmp(className, WC_EDIT) == 0 || wcscmp(className, MSFTEDIT_CLASS) == 0 || wcscmp(className, RICHEDIT_CLASS) == 0)
			{
				auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
				bool hasScrollBar = ((style & WS_HSCROLL) == WS_HSCROLL) || ((style & WS_VSCROLL) == WS_VSCROLL);
				if (p.theme && (hasScrollBar))
					//dark scrollbar for edit controls
					SetWindowTheme(hwnd, p.themeClassName, nullptr);

				return TRUE;
			}

#ifdef RICHEDIT_CLASS
			if (p.theme && wcscmp(className, RICHEDIT_CLASS) == 0)
			{
				SetWindowTheme(hwnd, p.themeClassName, nullptr);
				return TRUE;
			}
#endif

#ifdef MSFTEDIT_CLASS
			if (p.theme && wcscmp(className, MSFTEDIT_CLASS) == 0)
			{
				SetWindowTheme(hwnd, p.themeClassName, nullptr);
				return TRUE;
			}
#endif

#ifdef RICHEDIT60_CLASS
			if (p.theme && wcscmp(className, RICHEDIT60_CLASS) == 0)
			{
				SetWindowTheme(hwnd, p.themeClassName, nullptr);
				return TRUE;
			}
#endif
			if (wcscmp(className, WC_COMBOBOX) == 0)
			{
				auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);

				if ((style & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST || (style & CBS_DROPDOWN) == CBS_DROPDOWN)
				{
					COMBOBOXINFO cbi = {};
					cbi.cbSize = sizeof(COMBOBOXINFO);
					BOOL result = GetComboBoxInfo(hwnd, &cbi);
					if (result == TRUE)
					{
						if (p.theme && cbi.hwndList)
							//dark scrollbar for listbox of combobox
							SetWindowTheme(cbi.hwndList, p.themeClassName, nullptr);
					}

					if (p.subclass && PluginDarkMode::isEnabled())
						PluginDarkMode::subclassComboBoxControl(hwnd);
					if (p.subclass && !PluginDarkMode::isEnabled())
						RemoveWindowSubclass(hwnd, ComboBoxSubclass, g_comboBoxSubclassID);
				}
				return TRUE;
			}

			if (wcscmp(className, WC_LINK) == 0)
			{
				LITEM pItem;

				int i = 0;
				int found = true;

				// Iterate through all URLs in SysLink
				while (found)
				{
					pItem.iLink = i;
					pItem.mask = LIF_ITEMINDEX | LIF_STATE;

					found = static_cast<bool>(SendMessage(hwnd, LM_GETITEM, 0, reinterpret_cast<LPARAM>(&pItem)));
					if (found)
					{
						pItem.state = LIS_ENABLED | LIS_FOCUSED | (PluginDarkMode::isEnabled() ? LIS_DEFAULTCOLORS : 0);
						pItem.stateMask = LIS_ENABLED | LIS_FOCUSED | (PluginDarkMode::isEnabled() ? LIS_DEFAULTCOLORS : 0);
						SendMessage(hwnd, LM_SETITEM, 0, reinterpret_cast<LPARAM>(&pItem));
						i++;
					}
				}
			}

			if (p.theme && wcscmp(className, WC_LISTBOX) == 0)
			{
				SetWindowTheme(hwnd, p.themeClassName, nullptr); //dark scrollbar for listbox
				return TRUE;
			}

			if (wcscmp(className, WC_LISTVIEW) == 0)
			{
				HWND hHeader = ListView_GetHeader(hwnd);
				if (p.subclass && PluginDarkMode::isEnabled())
				{
					subclassListViewControl(hwnd);
					if (hHeader)
						subclassHeaderControl(hHeader);
				}

				if (p.theme)
				{
					SetWindowTheme(hwnd, p.themeClassName, nullptr);
					if (hHeader)
						SetWindowTheme(hHeader, PluginDarkMode::isEnabled() ? L"DarkMode_ItemsView" : nullptr, nullptr);
				}

				if (p.subclass && !PluginDarkMode::isEnabled())
				{
					RemoveWindowSubclass(hwnd, ListViewSubclass, g_listViewSubclassID);
					if (hHeader)
						RemoveWindowSubclass(hHeader, HeaderSubclass, g_listViewSubclassID);
				}
			}

			if (wcscmp(className, WC_TABCONTROL) == 0)
			{
				if (p.subclass && PluginDarkMode::isEnabled())
					subclassTabControl(hwnd);
				if (p.subclass && !PluginDarkMode::isEnabled())
					RemoveWindowSubclass(hwnd, TabSubclass, g_tabSubclassID);
			}

			if (wcscmp(className, WC_TREEVIEW) == 0)
			{
				calculateTreeViewStyle();
				setTreeViewStyle(hwnd);
			}

			if (wcscmp(className, UPDOWN_CLASS) == 0)
			{
				if (p.theme)
					PluginDarkMode::setDarkExplorerTheme(hwnd);

				if (p.subclass && PluginDarkMode::isEnabled())
					PluginDarkMode::subclassSpinControl(hwnd);
				if (p.subclass && !PluginDarkMode::isEnabled())
					RemoveWindowSubclass(hwnd, SpinControlSubclass, g_spinControlSubclassID);
			}

			return TRUE;
		}, reinterpret_cast<LPARAM>(&p));
	}

	void setDarkTitleBar(HWND hwnd)
	{
		PluginDarkMode::allowDarkModeForWindow(hwnd, PluginDarkMode::isEnabled());
		PluginDarkMode::setTitleBarThemeColor(hwnd);
	}

	void setDarkExplorerTheme(HWND hwnd)
	{
		SetWindowTheme(hwnd, PluginDarkMode::isEnabled() ? L"DarkMode_Explorer" : nullptr, nullptr);
	}

	void setDarkScrollBar(HWND hwnd)
	{
		PluginDarkMode::setDarkExplorerTheme(hwnd);
	}

	void setDarkTooltips(HWND hwnd, ToolTipsType type)
	{
		UINT msg = 0;
		switch (type)
		{
			case PluginDarkMode::ToolTipsType::toolbar:
				msg = TB_GETTOOLTIPS;
				break;
			case PluginDarkMode::ToolTipsType::listview:
				msg = LVM_GETTOOLTIPS;
				break;
			case PluginDarkMode::ToolTipsType::treeview:
				msg = TVM_GETTOOLTIPS;
				break;
			case PluginDarkMode::ToolTipsType::tabbar:
				msg = TCM_GETTOOLTIPS;
				break;
			default:
				msg = 0;
				break;
		}

		if (msg == 0)
		{
			PluginDarkMode::setDarkExplorerTheme(hwnd);
		}
		else
		{
			auto hTips = reinterpret_cast<HWND>(::SendMessage(hwnd, msg, 0, 0));
			if (hTips != nullptr)
			{
				PluginDarkMode::setDarkExplorerTheme(hTips);
			}
		}
	}

	void setDarkLineAbovePanelToolbar(HWND hwnd)
	{
		COLORSCHEME scheme;
		scheme.dwSize = sizeof(COLORSCHEME);

		if (PluginDarkMode::isEnabled())
		{
			scheme.clrBtnHighlight = PluginDarkMode::getDarkerBackgroundColor();
			scheme.clrBtnShadow = PluginDarkMode::getDarkerBackgroundColor();
		}
		else
		{
			scheme.clrBtnHighlight = CLR_DEFAULT;
			scheme.clrBtnShadow = CLR_DEFAULT;
		}

		::SendMessage(hwnd, TB_SETCOLORSCHEME, 0, reinterpret_cast<LPARAM>(&scheme));
	}

	void setDarkListView(HWND hwnd)
	{
		bool useDark = PluginDarkMode::isEnabled();

		HWND hHeader = ListView_GetHeader(hwnd);
		PluginDarkMode::allowDarkModeForWindow(hHeader, useDark);
		SetWindowTheme(hHeader, useDark ? L"ItemsView" : nullptr, nullptr);

		PluginDarkMode::allowDarkModeForWindow(hwnd, useDark);
		SetWindowTheme(hwnd, L"Explorer", nullptr);
	}

	void disableVisualStyle(HWND hwnd, bool doDisable)
	{
		if (doDisable)
		{
			SetWindowTheme(hwnd, L"", L"");
		}
		else
		{
			SetWindowTheme(hwnd, nullptr, nullptr);
		}
	}

	// range to determine when it should be better to use classic style
	constexpr double middleGrayRange = 2.0;

	void calculateTreeViewStyle()
	{
		COLORREF bgColor = PluginDarkMode::getBackgroundColor();

		if (treeViewBg != bgColor || lighnessTreeView == 50.0)
		{
			lighnessTreeView = calculatePerceivedLighness(bgColor);
			treeViewBg = bgColor;
		}

		if (lighnessTreeView < (50.0 - middleGrayRange))
		{
			treeViewStyle = TreeViewStyle::dark;
		}
		else if (lighnessTreeView > (50.0 + middleGrayRange))
		{
			treeViewStyle = TreeViewStyle::light;
		}
		else
		{
			treeViewStyle = TreeViewStyle::classic;
		}
	}

	void setTreeViewStyle(HWND hwnd)
	{
		auto style = static_cast<long>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		bool hasHotStyle = (style & TVS_TRACKSELECT) == TVS_TRACKSELECT;
		bool change = false;
		switch (treeViewStyle)
		{
		case TreeViewStyle::light:
		{
			if (!hasHotStyle)
			{
				style |= TVS_TRACKSELECT;
				change = true;
			}
			SetWindowTheme(hwnd, L"Explorer", nullptr);
			break;
		}
		case TreeViewStyle::dark:
		{
			if (!hasHotStyle)
			{
				style |= TVS_TRACKSELECT;
				change = true;
			}
			SetWindowTheme(hwnd, L"DarkMode_Explorer", nullptr);
			break;
		}
		default:
		{
			if (hasHotStyle)
			{
				style &= ~TVS_TRACKSELECT;
				change = true;
			}
			SetWindowTheme(hwnd, nullptr, nullptr);
			break;
		}
		}

		if (change)
		{
			::SetWindowLongPtr(hwnd, GWL_STYLE, style);
		}
	}

	void setBorder(HWND hwnd, bool border)
	{
		auto style = static_cast<long>(::GetWindowLongPtr(hwnd, GWL_STYLE));
		bool hasBorder = (style & WS_BORDER) == WS_BORDER;
		bool change = false;

		if (!hasBorder && border)
		{
			style |= WS_BORDER;
			change = true;
		}
		else if (hasBorder && !border)
		{
			style &= ~WS_BORDER;
			change = true;
		}

		if (change)
		{
			::SetWindowLongPtr(hwnd, GWL_STYLE, style);
			::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
		}
	}

	BOOL CALLBACK enumAutocompleteProc(HWND hwnd, LPARAM /*lParam*/)
	{
		constexpr size_t classNameLen = 16;
		TCHAR className[classNameLen]{};
		GetClassName(hwnd, className, classNameLen);
		if ((wcscmp(className, L"ListBoxX") == 0) ||
			(wcscmp(className, WC_LISTBOX) == 0))
		{
			PluginDarkMode::setDarkScrollBar(hwnd);
			::EnumChildWindows(hwnd, (WNDENUMPROC)enumAutocompleteProc, 0);
		}

		return TRUE;
	}

	// set dark scrollbar for autocomplete list
	void setDarkAutoCompletion()
	{
		::EnumThreadWindows(::GetCurrentThreadId(), (WNDENUMPROC)enumAutocompleteProc, 0);
	}

	LRESULT onCtlColor(HDC hdc)
	{
		if (!PluginDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, PluginDarkMode::getTextColor());
		::SetBkColor(hdc, PluginDarkMode::getBackgroundColor());
		return reinterpret_cast<LRESULT>(PluginDarkMode::getBackgroundBrush());
	}

	LRESULT onCtlColorSofter(HDC hdc)
	{
		if (!PluginDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, PluginDarkMode::getTextColor());
		::SetBkColor(hdc, PluginDarkMode::getSofterBackgroundColor());
		return reinterpret_cast<LRESULT>(PluginDarkMode::getSofterBackgroundBrush());
	}

	LRESULT onCtlColorDarker(HDC hdc)
	{
		if (!PluginDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, PluginDarkMode::getTextColor());
		::SetBkColor(hdc, PluginDarkMode::getDarkerBackgroundColor());
		return reinterpret_cast<LRESULT>(PluginDarkMode::getDarkerBackgroundBrush());
	}

	LRESULT onCtlColorError(HDC hdc)
	{
		if (!PluginDarkMode::isEnabled())
		{
			return FALSE;
		}

		::SetTextColor(hdc, PluginDarkMode::getTextColor());
		::SetBkColor(hdc, PluginDarkMode::getErrorBackgroundColor());
		return reinterpret_cast<LRESULT>(PluginDarkMode::getErrorBackgroundBrush());
	}
	
	LRESULT onCtlColorDarkerBGStaticText(HDC hdc, bool isTextEnabled)
	{
		if (!PluginDarkMode::isEnabled())
		{
			::SetTextColor(hdc, ::GetSysColor(isTextEnabled ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT));
			return FALSE;
		}

		::SetTextColor(hdc, isTextEnabled ? PluginDarkMode::getTextColor() : PluginDarkMode::getDisabledTextColor());
		::SetBkColor(hdc, PluginDarkMode::getDarkerBackgroundColor());
		return reinterpret_cast<LRESULT>(PluginDarkMode::getDarkerBackgroundBrush());
	}
}
