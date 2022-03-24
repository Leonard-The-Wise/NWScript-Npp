// Precompiled Header file.

#pragma once



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
#pragma warning (disable: 6011 26439 26451 26495 26812)  // LOTS of warnings...
#ifdef _W64
#define __w64 // get rid of retarded wxwidgets error
#include <wx/wx.h>
#undef __w64
#define __w64 __w64 
#else 
#include <wx/wx.h>
#endif
#pragma warning (pop)

#include "jpcre2.hpp"
#include "Nsc.h"
#include "tinyxml2.h"


