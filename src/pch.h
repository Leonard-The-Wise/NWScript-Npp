// Precompiled Header file.

#pragma once

#pragma comment (lib, "comctl32")          // Image List controls, tab control, etc
#pragma comment (lib, "shlwapi")           // Load icons from Shell Stock objects
#pragma comment (lib, "gdiplus")           // PNG file support

#include <Windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <commdlg.h>
#include <ShlObj.h>
#include <Shlwapi.h>
#include <gdiplus.h>
#include <Richedit.h>
#include <richole.h>

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

#include <thread>
#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

#include <tchar.h>
#include <assert.h>

#include "jpcre2.hpp"
#include "Nsc.h"
#include "tinyxml2.h"


namespace fs = std::filesystem;

