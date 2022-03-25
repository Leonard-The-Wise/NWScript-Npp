// Precompiled Header file.

#pragma once

#pragma comment (lib, "comctl32")          // Must use to create Image List controls
#pragma comment (lib, "shlwapi")           // Must use to load icons from Shell Stock objects
#pragma comment (lib, "rpcrt4")            // Link with wxwidgets

#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <ShlObj.h>
#include <Shlwapi.h>

#include <string>
#include <sstream>
#include <cwchar>
#include <locale>

#include <stdexcept>
#include <errhandlingapi.h>
#include <cctype>

#include <filesystem>
#include <fstream>
#include <iomanip>

#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include <tchar.h>
#include <assert.h>

#pragma warning (push)
#pragma warning (disable: 4251 6011 26439 26451 26495 26812)  // LOTS of warnings...
#ifdef _W64
#define __w64 // get rid of retarded wxwidgets error
#include <wx/wx.h>
#undef __w64
#define __w64 __w64 
#else 
#include <wx/wx.h>
#endif
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/hyperlink.h>
#include <wx/richtext/richtextctrl.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/imagpng.h>
#include <wx/frame.h>
#include <wx/tglbtn.h>
#include <wx/listctrl.h>
#include <wx/panel.h>
#include <wx/toolbar.h>
#pragma warning (pop)

#include "jpcre2.hpp"
#include "Nsc.h"
#include "tinyxml2.h"


