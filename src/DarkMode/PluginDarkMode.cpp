// This file is part of Notepad++ project
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

#define USE_PCH

#ifdef USE_PCH
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
		HBRUSH invertlightDarkerBackground = nullptr;
		HBRUSH invertlightSofterBackground = nullptr;

		Brushes(const Colors& colors)
			: background(::CreateSolidBrush(colors.background))
			, softerBackground(::CreateSolidBrush(colors.softerBackground))
			, hotBackground(::CreateSolidBrush(colors.hotBackground))
			, pureBackground(::CreateSolidBrush(colors.pureBackground))
			, errorBackground(::CreateSolidBrush(colors.errorBackground))
			, invertlightDarkerBackground(::CreateSolidBrush(invertLightness(colors.softerBackground)))
			, invertlightSofterBackground(::CreateSolidBrush(invertLightnessSofter(colors.softerBackground)))

		{}

		~Brushes()
		{
			::DeleteObject(background);			background = nullptr;
			::DeleteObject(softerBackground);	softerBackground = nullptr;
			::DeleteObject(hotBackground);		hotBackground = nullptr;
			::DeleteObject(pureBackground);		pureBackground = nullptr;
			::DeleteObject(errorBackground);	errorBackground = nullptr;
			::DeleteObject(invertlightDarkerBackground);	invertlightDarkerBackground = nullptr;
			::DeleteObject(invertlightSofterBackground);	invertlightSofterBackground = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(background);
			::DeleteObject(softerBackground);
			::DeleteObject(hotBackground);
			::DeleteObject(pureBackground);
			::DeleteObject(errorBackground);
			::DeleteObject(invertlightDarkerBackground);
			::DeleteObject(invertlightSofterBackground);

			background = ::CreateSolidBrush(colors.background);
			softerBackground = ::CreateSolidBrush(colors.softerBackground);
			hotBackground = ::CreateSolidBrush(colors.hotBackground);
			pureBackground = ::CreateSolidBrush(colors.pureBackground);
			errorBackground = ::CreateSolidBrush(colors.errorBackground);
			invertlightDarkerBackground = ::CreateSolidBrush(invertLightness(colors.background));
			invertlightSofterBackground = ::CreateSolidBrush(invertLightnessSofter(colors.background));
		}
	};

	struct Pens
	{
		HPEN darkerTextPen = nullptr;
		HPEN edgePen = nullptr;
		HPEN lightEdgePen = nullptr;

		Pens(const Colors& colors)
			: darkerTextPen(::CreatePen(PS_SOLID, 1, colors.darkerText))
			, edgePen(::CreatePen(PS_SOLID, 1, colors.edge))
			, lightEdgePen(::CreatePen(PS_SOLID, 1, invertLightness(colors.darkerText)))
		{}

		~Pens()
		{
			::DeleteObject(darkerTextPen);	darkerTextPen = nullptr;
			::DeleteObject(edgePen);		edgePen = nullptr;
			::DeleteObject(lightEdgePen);	lightEdgePen = nullptr;
		}

		void change(const Colors& colors)
		{
			::DeleteObject(darkerTextPen);
			::DeleteObject(edgePen);
			::DeleteObject(lightEdgePen);

			darkerTextPen = ::CreatePen(PS_SOLID, _dpiManager.scaleX(1), colors.darkerText);
			edgePen = ::CreatePen(PS_SOLID, _dpiManager.scaleX(1), colors.edge);
			lightEdgePen = ::CreatePen(PS_SOLID, 1, invertLightness(colors.softerBackground));
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

	void initDarkMode()
	{
		_options = configuredOptions(false);

		initExperimentalDarkMode();
		setDarkMode(false, true);
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

		l = 240 - l;

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

	HBRUSH getBackgroundBrush()           { return getTheme()._brushes.background; }
	HBRUSH getSofterBackgroundBrush()     { return getTheme()._brushes.softerBackground; }
	HBRUSH getHotBackgroundBrush()        { return getTheme()._brushes.hotBackground; }
	HBRUSH getDarkerBackgroundBrush()     { return getTheme()._brushes.pureBackground; }
	HBRUSH getErrorBackgroundBrush()      { return getTheme()._brushes.errorBackground; }
	HBRUSH getInvertlightDarkerBackgroundBrush() { return getTheme()._brushes.invertlightDarkerBackground; }
	HBRUSH getInvertlightSofterBackgroundBrush() { return getTheme()._brushes.invertlightSofterBackground; }

	HPEN getDarkerTextPen()               { return getTheme()._pens.darkerTextPen; }
	HPEN getEdgePen()                     { return getTheme()._pens.edgePen; }
	HPEN getLightEdgePen()				  { return getTheme()._pens.lightEdgePen; }

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

	// Which classes exist for themes?
	// https://stackoverflow.com/questions/217532/what-are-the-possible-classes-for-the-openthemedata-function
	struct ButtonData
	{
		HTHEME hTheme = nullptr;
		int iStateID = 0;
		HBITMAP hbmMask = nullptr;

		~ButtonData()
		{
			closeTheme();
			if (hbmMask)
				DeleteObject(hbmMask);
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

	void renderButton(HWND hwnd, HDC hdc, HTHEME hTheme, int iPartID, int iStateID)
	{
		RECT rcClient = {};
		WCHAR szText[256] = { '\0' };
		DWORD nState = static_cast<DWORD>(SendMessage(hwnd, BM_GETSTATE, 0, 0));
		DWORD uiState = static_cast<DWORD>(SendMessage(hwnd, WM_QUERYUISTATE, 0, 0));
		LONG_PTR nStyle = GetWindowLongPtr(hwnd, GWL_STYLE);

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

		if (iPartID == 1)
		{
			dtFlags &= ~(DT_RIGHT);
			dtFlags |= DT_CENTER;
		}

		if (!(nStyle & BS_MULTILINE) && !(nStyle & BS_BOTTOM) && !(nStyle & BS_TOP))
		{
			dtFlags |= DT_VCENTER;
		}

		GetClientRect(hwnd, &rcClient);
		GetWindowText(hwnd, szText, _countof(szText));

		SIZE szBox = { 13, 13 };
		GetThemePartSize(hTheme, hdc, iPartID, iStateID, NULL, TS_DRAW, &szBox);
		if (iPartID == 1)
		{
			szBox.cx = rcClient.right;
			szBox.cy = rcClient.bottom;
		}

		RECT rcText = rcClient;
		GetThemeBackgroundContentRect(hTheme, hdc, iPartID, iStateID, &rcClient, &rcText);

		RECT rcBackground = rcClient;
		if (dtFlags & DT_SINGLELINE)
		{
			if (iPartID > 1)
				rcBackground.top += (rcText.bottom - rcText.top - szBox.cy) / 2;
		}
		rcBackground.bottom = rcBackground.top + szBox.cy;
		rcBackground.right = rcBackground.left + szBox.cx;

		if (iPartID > 1)
			rcText.left = rcBackground.right + 3;

		if (IsThemeBackgroundPartiallyTransparent(hTheme, iPartID, iStateID))
			DrawThemeParentBackground(hwnd, hdc, &rcClient);
		if (iPartID == 1)
		{
			DWORD nState = static_cast<DWORD>(SendMessage(hwnd, BM_GETSTATE, 0, 0));
			HBRUSH hBckBrush = ((nState & BST_HOT) != 0) ? PluginDarkMode::getInvertlightSofterBackgroundBrush() : PluginDarkMode::getDarkerBackgroundBrush();
			if ((nState & BST_PUSHED) != 0 || ((nState & BST_CHECKED) != 0))
				hBckBrush = PluginDarkMode::getSofterBackgroundBrush();

			if (nStyle & WS_DISABLED)
				SelectObject(hdc, PluginDarkMode::getDarkerTextPen());
			else if ((nState & BST_FOCUS) || (nStyle & BS_DEFPUSHBUTTON))
				SelectObject(hdc, PluginDarkMode::getLightEdgePen());
			else 
				SelectObject(hdc, PluginDarkMode::getEdgePen());

			SelectObject(hdc, hBckBrush);
			RoundRect(hdc, rcClient.left, rcClient.top, rcClient.right, rcClient.bottom, _dpiManager.scaleX(5), _dpiManager.scaleY(5));
		}
		else
			DrawThemeBackground(hTheme, hdc, iPartID, iStateID, &rcBackground, nullptr);

		// Draw button image
		RECT rcImage = rcClient;
		if (iPartID == 1)
		{
			// Calculate actual text output rectangle and centralize
			int padding = _dpiManager.scaleX(4);
			DrawText(hdc, szText, std::wstring(szText).size(), &rcImage, DT_CALCRECT);
			rcImage.left = padding + (rcClient.right - rcImage.right) / 2;
			rcImage.right += padding + rcImage.left;

			ICONINFO ii;
			BITMAP bm;

			HICON hIcon = reinterpret_cast<HICON>(SendMessage(hwnd, BM_GETIMAGE, IMAGE_ICON, 0));
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
		}

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

			if (iPartID == 1)
			{
				rcClient.left += _dpiManager.scaleX(2); rcClient.right -= _dpiManager.scaleX(2);
				rcClient.top += _dpiManager.scaleY(2); rcClient.bottom -= _dpiManager.scaleY(2);
				DrawFocusRect(hdc, &rcClient);
			}
			else
				DrawFocusRect(hdc, &rcFocus);
		}		

		if (hCreatedFont) DeleteObject(hCreatedFont);
		SelectObject(hdc, hOldFont);
	}

	void paintButton(HWND hwnd, HDC hdc, ButtonData& buttonData)
	{
		DWORD nState = static_cast<DWORD>(SendMessage(hwnd, BM_GETSTATE, 0, 0));
		LONG_PTR nStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		LONG_PTR nUserData = GetWindowLongPtr(hwnd, GWLP_USERDATA);
		DWORD nButtonStyle = nStyle & 0xF;
		bool bNormalButton = false;

		DWORD PushLike = nStyle & BS_PUSHLIKE;

		int iPartID = BP_CHECKBOX;
		if ((nStyle & BS_PUSHLIKE) || (nUserData == BS_PUSHLIKE))
		{
			iPartID = BP_PUSHBUTTON;
			if (nUserData == BS_PUSHLIKE)
				bNormalButton = true;
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

		if ((nState & BST_CHECKED) && !bNormalButton)		iStateID += 4;

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
				renderButton(hwnd, hdcFrom, buttonData.hTheme, iPartID, buttonData.iStateID);
			}
			if (hdcTo)
			{
				renderButton(hwnd, hdcTo, buttonData.hTheme, iPartID, iStateID);
			}

			buttonData.iStateID = iStateID;

			EndBufferedAnimation(hbpAnimation, TRUE);
		}
		else
		{
			renderButton(hwnd, hdc, buttonData.hTheme, iPartID, iStateID);

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
						hdc = BeginPaint(hWnd, &ps);

					ULONG_PTR token = 0;
					Gdiplus::GdiplusStartupInput input = NULL;
					Gdiplus::GdiplusStartup(&token, &input, NULL);

					paintButton(hWnd, hdc, *pButtonData);

					if (ps.hdc)
						EndPaint(hWnd, &ps);

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

	LRESULT DrawHeaderItem(HWND hHeader, HDC hdc, RECT& rcItem, int itemID)
	{
		constexpr const TCHAR sortArrowDown[2] = L"˅";
		constexpr const TCHAR sortArrowUp[2] = L"˄";

		POINT cursosPos;
		GetCursorPos(&cursosPos);
		ScreenToClient(hHeader, &cursosPos);

		// Information on Control Style
		LONG_PTR headerStyle = GetWindowLongPtr(hHeader, GWL_STYLE);
		bool clickableHeaderStyle = (headerStyle & HDS_BUTTONS) == HDS_BUTTONS;

		// Information on Header Item
		HDITEM hdItem = {};
		TCHAR buffer[MAX_PATH] = { '/0' };
		hdItem.mask = HDI_TEXT | HDI_FORMAT;
		hdItem.pszText = buffer;
		hdItem.cchTextMax = std::size(buffer);
		Header_GetItem(hHeader, itemID, &hdItem);

		// Temporary modifiable rectangle for item
		RECT txtRC;
		CopyRect(&txtRC, &rcItem);

		HFONT ArrowFont = nullptr;
		if (hdItem.fmt & (HDF_SPLITBUTTON | HDF_SORTDOWN | HDF_SORTUP))
			ArrowFont = ::CreateFont(_dpiManager.scaleX(10), _dpiManager.scaleY(10), 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
				OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, TEXT("Courier New"));

		SetTextColor(hdc, PluginDarkMode::getDarkerTextColor());
		SetBkMode(hdc, TRANSPARENT);

		// Get current hot item if appliable
		bool vItemPressed = false, vItemFocused = false;
		if (clickableHeaderStyle)
		{
			HDHITTESTINFO hti = {};
			hti.pt = cursosPos;
			SendMessage(hHeader, HDM_HITTEST, 0, reinterpret_cast<LPARAM>(&hti));
			int hotItem = hti.iItem;
			vItemFocused = (hotItem == itemID);
			vItemPressed = (vItemFocused && (GetKeyState(VK_LBUTTON) & 0x8000) != 0);

			// Draw background
			if (vItemPressed || vItemFocused)
				FillRect(hdc, &rcItem, getInvertlightSofterBackgroundBrush());

			// Draw Combo box if appliable
			if ((hdItem.fmt & HDF_SPLITBUTTON) && (vItemPressed || vItemFocused))
			{
				// Draw combo arrow
				bool comboHit = ((hti.flags & HHT_ONDROPDOWN) != 0);
				if (comboHit)
					SetTextColor(hdc, PluginDarkMode::getTextColor());
				
				RECT splitRc;
				CopyRect(&splitRc, &rcItem);
				splitRc.top = splitRc.bottom / 3;
				splitRc.right += _dpiManager.scaleX(1);
				splitRc.left = splitRc.right - _dpiManager.scaleX(18);
				HFONT hOldFont = nullptr;
				if (ArrowFont)
					hOldFont = reinterpret_cast<HFONT>(SelectObject(hdc, ArrowFont));
				DrawText(hdc, sortArrowDown, std::size(sortArrowDown), &splitRc, DT_NOPREFIX | DT_TOP | DT_CENTER);

				if (comboHit)
					SetTextColor(hdc, PluginDarkMode::getDarkerTextColor());

				// Draw box splitter
				splitRc.left -= _dpiManager.scaleX(4);
				auto hOldPen = static_cast<HPEN>(::SelectObject(hdc, PluginDarkMode::getEdgePen()));
				POINT comboEdges[] = {
					{ splitRc.left - _dpiManager.scaleX(1), rcItem.top},
					{ splitRc.left - _dpiManager.scaleX(1), rcItem.bottom}
				};
				Polyline(hdc, comboEdges, _countof(comboEdges));
				SelectObject(hdc, hOldPen);
				if (hOldFont)
					SelectObject(hdc, hOldFont);
			}
		}

		// Draw Sort arrow
		if (hdItem.fmt & (HDF_SORTDOWN | HDF_SORTUP))
		{
			HFONT hOldFont;
			if (ArrowFont)
				hOldFont = reinterpret_cast<HFONT>(SelectObject(hdc, ArrowFont));

			if (hdItem.fmt & HDF_SORTDOWN)
				DrawText(hdc, sortArrowDown, std::size(sortArrowDown), &txtRC, DT_NOPREFIX | DT_TOP | DT_CENTER);
			else 
				DrawText(hdc, sortArrowUp, std::size(sortArrowUp), &txtRC, DT_NOPREFIX | DT_TOP | DT_CENTER);

			if (hOldFont)
				SelectObject(hdc, hOldFont);
		}

		if (ArrowFont)
			DeleteObject(ArrowFont);

		SetTextColor(hdc, PluginDarkMode::getDarkerTextColor());

		DWORD textFormat;
		textFormat = (hdItem.fmt & HDF_LEFT) ? DT_LEFT : 0;
		if (textFormat == 0)
			textFormat = (hdItem.fmt & HDF_CENTER) ? DT_CENTER : 0;
		if (textFormat == 0)
			textFormat = (hdItem.fmt & HDF_RIGHT) ? DT_RIGHT : 0;
		textFormat |= DT_NOPREFIX | DT_VCENTER | DT_END_ELLIPSIS;

		txtRC.left += 5;
		txtRC.top += 4;
		if (vItemPressed)
		{
			txtRC.left += _dpiManager.scaleX(1);
			txtRC.top += _dpiManager.scaleY(1);
			if (hdItem.fmt & HDF_SPLITBUTTON)
				txtRC.right -= _dpiManager.scaleX(23);
		}

		buffer[MAX_PATH - 1] = '\0';
		DrawText(hdc, std::wstring(buffer).c_str(), std::wstring(buffer).size(), &txtRC, textFormat);

		// Draw grid lines
		auto hOldPen = static_cast<HPEN>(::SelectObject(hdc, PluginDarkMode::getEdgePen()));
		POINT edges[] = {
			{rcItem.right - _dpiManager.scaleX(1), rcItem.top},
			{rcItem.right - _dpiManager.scaleX(1), rcItem.bottom}
		};
		Polyline(hdc, edges, _countof(edges));
		::SelectObject(hdc, hOldPen);

		// TODO: 
		// Handle Header styles: HDS_HORZ, HDS_FILTERBAR? (https://devblogs.microsoft.com/oldnewthing/20120227-00/?p=8223)
		// Handle HDITEM styles and formats: HDI_BITMAP, HDI_IMAGE, HDF_CHECKBOX

		return CDRF_SKIPDEFAULT;
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
		switch (uMsg)
		{
			case WM_PAINT:
			{
				PAINTSTRUCT ps;
				HDC hdc = BeginPaint(hWnd, &ps);

				RECT rcClient = {};
				GetClientRect(hWnd, &rcClient);
				FillRect(hdc, &rcClient, getDarkerBackgroundBrush());

				HFONT lstFont = reinterpret_cast<HFONT>(::SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0));
				HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, lstFont));

				int count = static_cast<int>(Header_GetItemCount(hWnd));
				RECT wRc = {};
				for (int i = 0; i < count; i++)
				{
					Header_GetItemRect(hWnd, i, &wRc);
					DrawHeaderItem(hWnd, hdc, wRc, i);
				}

				SelectObject(hdc, oldFont);
				EndPaint(hWnd, &ps);

				return TRUE;
			}
		}

		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}

	void subclassHeaderControl(HWND hwndHeader)
	{
		SetWindowSubclass(hwndHeader, HeaderSubclass, g_headerSubclassID, 0);
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
#ifdef USE_CUSTOMDRAW
		// This actually gets notifications from Listview Header, since ListView is the parent of the header
		case WM_NOTIFY:
		{
			LPNMHDR nmhdr = reinterpret_cast<LPNMHDR>(lParam);
			if (nmhdr->code == NM_CUSTOMDRAW)
			{
				LPNMCUSTOMDRAW lpcd = reinterpret_cast<LPNMCUSTOMDRAW>(lParam);
				switch (lpcd->dwDrawStage)
				{
				case CDDS_PREPAINT:
				{
					if (!PluginDarkMode::isEnabled())
						return CDRF_DODEFAULT;

					FillRect(lpcd->hdc, &lpcd->rc, getDarkerBackgroundBrush());
					return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
				}
				case CDDS_ITEMPREPAINT:
				{
					return DrawHeaderItem(lParam);
				}

				case CDDS_POSTPAINT:

					// Calculates the undrawn border outside columns
					HWND hHeader = lpcd->hdr.hwndFrom;
					int count = static_cast<int>(Header_GetItemCount(hHeader));
					int colsWidth = 0;
					RECT wRc = {};
					for (int i = 0; i < count; i++)
					{
						Header_GetItemRect(hHeader, i, &wRc);
						colsWidth += wRc.right - wRc.left;
					}

					RECT clientRect;
					GetClientRect(hHeader, &clientRect);
					if (clientRect.right > (colsWidth + dpiManager().scaleX(3)))
					{
						clientRect.left = colsWidth + dpiManager().scaleX(1);
						HDC hdc = GetDC(hHeader);
						FillRect(hdc, &clientRect, getDarkerBackgroundBrush());
						ReleaseDC(hHeader, hdc);
						RedrawWindow(hHeader, NULL, NULL, RDW_UPDATENOW);
					}

					return CDRF_SKIPDEFAULT;
				}
			}
			break;
		}
#endif

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

			SendMessageW(hHeader, WM_THEMECHANGED, wParam, lParam);
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

	constexpr UINT_PTR g_staticSubclassID = 42;

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
				COLORREF c = PluginDarkMode::getLinkTextColor();
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
				case BS_CHECKBOX:
				case BS_AUTOCHECKBOX:
				case BS_RADIOBUTTON:
				case BS_AUTORADIOBUTTON:
				{
					auto nButtonAllStyles = ::GetWindowLongPtr(hwnd, GWL_STYLE);
					if (nButtonAllStyles & BS_PUSHLIKE)
					{
						if (p.theme)
							SetWindowTheme(hwnd, p.themeClassName, nullptr);
						if (p.subclass)
							PluginDarkMode::subclassButtonControl(hwnd);
						break;
					}

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
				case BS_DEFPUSHBUTTON:
				case BS_PUSHBUTTON:
					if (p.theme)
						SetWindowTheme(hwnd, p.themeClassName, nullptr);
					if (p.subclass)
					{
						SetWindowLongPtr(hwnd, GWLP_USERDATA, BS_PUSHLIKE);
						PluginDarkMode::subclassButtonControl(hwnd);
					}

					break;
				}
				return TRUE;
			}

			if (wcscmp(className, WC_EDIT) == 0 || wcscmp(className, MSFTEDIT_CLASS) == 0 || wcscmp(className, RICHEDIT_CLASS) == 0)
			{
				auto style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
				bool hasScrollBar = ((style & WS_HSCROLL) == WS_HSCROLL) || ((style & WS_VSCROLL) == WS_VSCROLL);
				if (p.theme && (hasScrollBar || wcscmp(className, MSFTEDIT_CLASS) == 0 || wcscmp(className, RICHEDIT_CLASS) == 0))
					//dark scrollbar for edit and richedit controls
					SetWindowTheme(hwnd, p.themeClassName, nullptr);

				return TRUE;
			}

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

			if (wcscmp(className, WC_LISTBOX) == 0)
			{
				if (p.theme)
					SetWindowTheme(hwnd, p.themeClassName, nullptr); //dark scrollbar for listbox

				return TRUE;
			}

			if (wcscmp(className, WC_LISTVIEW) == 0)
			{
				if (p.subclass && PluginDarkMode::isEnabled())
				{
					subclassListViewControl(hwnd);
					subclassHeaderControl(ListView_GetHeader(hwnd));
				}

				HWND hHeader = ListView_GetHeader(hwnd);
				if (p.theme)
				{
					if (hHeader)
						SetWindowTheme(hHeader, PluginDarkMode::isEnabled() ? L"DarkMode_ItemsView" : nullptr, nullptr);
					SetWindowTheme(hwnd, PluginDarkMode::isEnabled() ? L"DarkMode_ItemsView" : nullptr, nullptr);
				}

				if (p.subclass && !PluginDarkMode::isEnabled())
				{
					if (hHeader)
						RemoveWindowSubclass(hHeader, HeaderSubclass, g_listViewSubclassID);
					RemoveWindowSubclass(hwnd, ListViewSubclass, g_listViewSubclassID);
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
