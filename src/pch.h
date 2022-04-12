// Precompiled Header file.

#pragma once

#pragma comment (lib, "comctl32")          // Image List controls, tab control, etc
#pragma comment (lib, "shlwapi")           // Load icons from Shell Stock objects
#pragma comment (lib, "gdiplus")           // PNG file support

#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <objidl.h>
#include <ShlObj.h>
#include <Shlwapi.h>

// Bug with GDI+ include...
// https://developercommunity.visualstudio.com/t/gdiplustypesh-does-not-compile-with-nominmax/727770
#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define CUSTOM_MAX
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#define CUSTOM_MIN
#endif
#include <gdiplus.h>
#ifdef CUSTOM_MAX
#undef max
#undef CUSTOM_MAX
#endif
#ifdef CUSTOM_MIN
#undef min
#undef CUSTOM_MIN
#endif



#include <Richedit.h>
#include <richole.h>

#include <string>
#include <sstream>
#include <cwchar>
#include <locale>
#include <format>

#include <stdexcept>
#include <errhandlingapi.h>
#include <cctype>

#include <filesystem>
#include <fstream>
#include <iomanip>

#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>

#include <thread>
#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>
#include <set>

#include <tchar.h>
#include <assert.h>

#include "jpcre2.hpp"
#include "tinyxml2.h"




namespace fs = std::filesystem;

