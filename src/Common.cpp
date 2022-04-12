/** @file Common.h
 * Definitions for common types.
 *
 * Remarks: MUST include shlwapi.lib reference for Linker dependencies
 * in order to be able to build projects using this file.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "pch.h"
//#include <assert.h>
//#include <windows.h>
//#include <commdlg.h>
//#include <shellapi.h>
//#include <ShlObj.h>
//#include <Shlwapi.h>
//#include <stdexcept>
//#include <errhandlingapi.h>
//#include <sstream>
//#include <tchar.h>
//#include <fstream>
//#include "tinyxml2.h"

#include "Common.h"


namespace NWScriptPluginCommons {

    // Formats numeric type I into hexadecimal format
    // Extracted from here: https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
    template <typename I>
    std::string n2hexstr(I w, size_t hex_len) {
        static const char* digits = "0123456789ABCDEF";
        std::string rc(hex_len, '0');
        for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
            rc[i] = digits[(w >> j) & 0x0f];
        return "0x" + rc;
    }

    // Formats numeric type I into hexadecimal format (TCHAR version)
    // Extracted from here: https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
    template <typename I>
    generic_string n2hexstrg(I w, size_t hex_len) {
        static const TCHAR* digits = TEXT("0123456789ABCDEF");
        generic_string rc(hex_len, TEXT('0'));
        for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
            rc[i] = digits[(w >> j) & 0x0f];
        return TEXT("0x") + rc;
    }

    // Since codecvt is now deprecated API and no replacement is provided, we write our own.
    std::wstring str2wstr(const std::string& string)
    {
        if (string.empty())
        {
            return L"";
        }

        const auto size_needed = MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), nullptr, 0);
        if (size_needed < 1)
        {
            throw std::runtime_error("MultiByteToWideChar() failed: " + std::to_string(size_needed));
        }

        std::wstring result(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &string.at(0), (int)string.size(), &result.at(0), size_needed);
        return result;
    }

    // Since codecvt is now deprecated API and no replacement is provided, we write our own.
    std::string wstr2str(const std::wstring& wide_string)
    {
        if (wide_string.empty())
        {
            return "";
        }

        const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), nullptr, 0, nullptr, nullptr);
        if (size_needed < 1)
        {
            throw std::runtime_error("WideCharToMultiByte() failed: " + std::to_string(size_needed));
        }

        std::string result(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wide_string.at(0), (int)wide_string.size(), &result.at(0), size_needed, nullptr, nullptr);
        return result;
    }

    // Returns a new string replacing all input string %VARIABLES% variables with the associated string map(%VARIABLE%, %VALUE%)
    std::string replaceStringsA(const std::string& input, std::map<std::string, std::string>& replaceStrings)
    {
        typedef jpcre2::select<char> pcre2;
        std::string output = input;

        for (std::map<std::string, std::string>::const_iterator i = replaceStrings.begin(); i != replaceStrings.end(); i++)
        {
            output = pcre2::Regex(i->first.c_str()).replace(output, i->second.c_str(), "g");
        }

        return output;
    }

    // Returns a new string replacing all input string %VARIABLES% variables with the associated string map(%VARIABLE%, %VALUE%)
    std::wstring replaceStringsW(const std::wstring& input, std::map<std::wstring, std::wstring>& replaceStrings)
    {
        typedef jpcre2::select<wchar_t> pcre2;
        std::wstring output = input;

        for (std::map<std::wstring, std::wstring>::const_iterator i = replaceStrings.begin(); i != replaceStrings.end(); i++)
        {
            output = pcre2::Regex(i->first.c_str()).replace(output, i->second.c_str(), "g");
        }

        return output;
    }

    // Opens a file dialog
    bool openFileDialog(HWND hOwnerWnd, std::vector<generic_string>& outSelectedFiles, const TCHAR* sFilters,
        const generic_string& lastOpenedFolder, bool multiFile)
    {
        OPENFILENAME ofn;

        std::unique_ptr<TCHAR[]> sFileOpen = std::make_unique<TCHAR[]>(MAX_PATH * 256);
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hOwnerWnd;
        ofn.lpstrFile = sFileOpen.get();
        ofn.lpstrFile[0] = L'\0';
        ofn.nMaxFile = MAX_PATH * sizeof(TCHAR) * 256;
        ofn.lpstrFilter = sFilters;
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = multiFile ? reinterpret_cast<LPWSTR>(generic_string(TEXT("Select one or more files")).data()) 
            : reinterpret_cast<LPWSTR>(generic_string(TEXT("Select file")).data());
        ofn.nMaxFileTitle = 0;
        ofn.lpstrInitialDir = lastOpenedFolder.c_str();
        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | (multiFile ? OFN_ALLOWMULTISELECT : 0);

        if (!GetOpenFileName(&ofn))
        {
            DWORD nFail = CommDlgExtendedError();

            if (nFail == 0)
                return false;

            if (nFail == 0x3003)
            {
                generic_stringstream sError;
                sError << TEXT("You have selected too many files! Current buffer can support a maximum of ") <<
                    MAX_PATH * 256 << TEXT(" characters for file selection. Please select less files the next time.");
                MessageBox(hOwnerWnd, sError.str().c_str(), TEXT("NWScript Plugin Error"), MB_OK | MB_ICONEXCLAMATION| MB_APPLMODAL);
            }
            else
            {
                generic_stringstream sError;
                sError << TEXT("Error opening dialog box. Please report it to the plugin creator!\r\n") <<
                    TEXT("Error code: ") << n2hexstrg(nFail, 4).c_str();
                MessageBox(hOwnerWnd, sError.str().c_str(), TEXT("NWScript Plugin Error"), MB_OK | MB_ICONERROR | MB_APPLMODAL);
            }

            return false;
        }

        if (!multiFile)
            outSelectedFiles.push_back(sFileOpen.get());
        else
        {
            TCHAR* pFiles = sFileOpen.get();
            generic_string path = pFiles;
            pFiles += path.size() + 1;

            if (!PathIsDirectory(path.c_str()))
                outSelectedFiles.push_back(path.c_str());
            else
            {
                for (; sFileOpen != 0; )
                {
                    generic_string fileName = pFiles;
                    generic_string newPath = path + TEXT("\\") + fileName;
                    if (!PathIsDirectory(newPath.c_str()) && PathFileExists(newPath.c_str()))
                        outSelectedFiles.push_back(path + TEXT("\\") + fileName);
                    else
                        break;
                    pFiles += fileName.size() + 1;
                }
            }
        }

        return true;
    }

    // Opens a folder selection dialog
    bool openFolderDialog(HWND hOwnerWnd, generic_string& outFolderName, const generic_string& startPath,
        UINT flags)
    {
        BROWSEINFO b = { 0 };
        LPITEMIDLIST pidlSelected;
        b.hwndOwner = hOwnerWnd;
        b.lpszTitle = TEXT("Select Folder");
        b.ulFlags = flags;
        b.lpfn = BrowseCallbackProc;
        b.lParam = reinterpret_cast<LPARAM>(startPath.c_str());
        pidlSelected = SHBrowseForFolder(&b);
        TCHAR tszPath[MAX_PATH] = TEXT("");

        if (SHGetPathFromIDList(pidlSelected, tszPath) == TRUE)
        {
            outFolderName = tszPath;
            return true;
        }

        return false;
    }

    // No need for warning of enum here
#pragma warning(push)
#pragma warning(disable : 26812)

    // Retrieves an HICON from the standard Windows libraries
    HICON getStockIcon(SHSTOCKICONID stockIconID, IconSize iconSize)
    {
        UINT uiFlags = SHGSI_ICONLOCATION;

        SHSTOCKICONINFO sii;
        ZeroMemory(&sii, sizeof(sii));
        sii.cbSize = sizeof(sii);
        HRESULT hResult = SHGetStockIconInfo(stockIconID, uiFlags, &sii);

        if (!SUCCEEDED(hResult))
            return NULL;

        HMODULE hModule = LoadLibrary(sii.szPath);
        HICON hIcon = NULL;

        if (hModule)
        {
            hIcon = reinterpret_cast<HICON>(LoadImage(hModule, MAKEINTRESOURCE(-sii.iIcon),
                IMAGE_ICON, iconSize, iconSize, LR_DEFAULTCOLOR));
            FreeLibrary(hModule);
        }

        return hIcon;
    }

    // Converts a HICON to HBITMAP, preserving transparency channels
    // Extracted from example here:
    // https://cpp.hotexamples.com/pt/examples/-/-/DrawIconEx/cpp-drawiconex-function-examples.html
    HBITMAP iconToBitmap(HICON hIcon)
    {
        if (!hIcon)
            return NULL;

        ICONINFO iconInfo;
        ZeroMemory(&iconInfo, sizeof(iconInfo));
        GetIconInfo(hIcon, &iconInfo);

        HDC screenDevice = GetDC(NULL);
        HDC hdc = CreateCompatibleDC(screenDevice);
        ReleaseDC(NULL, screenDevice);

        int iconWidth = iconInfo.xHotspot * 2;
        int iconHeight = iconInfo.yHotspot * 2;

        BITMAPINFOHEADER bitmapInfoH = { sizeof(BITMAPINFOHEADER), iconWidth, iconHeight, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
        BITMAPINFO bitmapInfo = { bitmapInfoH, {} };
        HBITMAP winBitmap = nullptr;
        HGDIOBJ oldHdc = nullptr;
        DWORD* bits = nullptr;

        winBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, (VOID**)&bits, NULL, 0);
        if (winBitmap)
        {
            // Switch hdc object to point into the bitmap.
            oldHdc = SelectObject(hdc, reinterpret_cast<HGDIOBJ>(winBitmap));
            if (oldHdc)
                DrawIconEx(hdc, 0, 0, hIcon, iconWidth, iconHeight, 0, 0, DI_NORMAL);
        }

        DeleteObject(iconInfo.hbmMask);
        DeleteObject(iconInfo.hbmColor);

        // Restore previous object (state) for HDC, like recommended here: 
        // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/nf-wingdi-selectobject
        if (oldHdc)
            SelectObject(hdc, oldHdc);

        DeleteDC(hdc);

        return winBitmap;
    }

    // Retrieves an HICON from the standard Windows libraries and convert it to a Device Independent Bitmap
    HBITMAP getStockIconBitmap(SHSTOCKICONID stockIconID, IconSize iconSize) {
        return iconToBitmap(getStockIcon(stockIconID, iconSize));
    }

#pragma warning (push)
#pragma warning (disable : 6387)
    // Load a PNG from resources and convert into an HBITMAP.
    HBITMAP loadPNGFromResource(HMODULE module, int idResource)
    {
        HBITMAP retval = NULL;
        ULONG_PTR token = 0;
        Gdiplus::GdiplusStartupInput input = NULL;
        Gdiplus::GdiplusStartup(&token, &input, NULL);

        if (token != 0)
        {
            // Load resource
            auto hResource = FindResourceW(module, MAKEINTRESOURCE(idResource), L"PNG");
            size_t _size = SizeofResource(module, hResource);
            auto hMemory = LoadResource(module, hResource);
            LPVOID ptr = LockResource(hMemory);

            // copy image bytes into a real hglobal memory handle
            hMemory = ::GlobalAlloc(GHND, _size);
            if (hMemory)
            {
                void* pBuffer = ::GlobalLock(hMemory);
                memcpy(pBuffer, ptr, _size);
            }

            // Create stream
            IStream* pStream = nullptr;
            HRESULT hr = CreateStreamOnHGlobal(hMemory, TRUE, &pStream);
            if (SUCCEEDED(hr))
            {
                Gdiplus::Bitmap* bmp = new Gdiplus::Bitmap(pStream);
                bmp->GetHBITMAP(Gdiplus::Color::Transparent, &retval);
                delete bmp;
                Gdiplus::GdiplusShutdown(token);
                pStream->Release();
            }

            if (hMemory)
                GlobalFree(hMemory);
        }

        return retval;
    }

    // Load a PNG from resources and convert into an HICON.
    HICON loadPNGFromResourceIcon(HMODULE module, int idResource)
    {
        HICON retval = NULL;
        ULONG_PTR token = 0;
        Gdiplus::GdiplusStartupInput input = NULL;
        Gdiplus::GdiplusStartup(&token, &input, NULL);

        if (token != 0)
        {
            // Load resource
            auto hResource = FindResourceW(module, MAKEINTRESOURCE(idResource), L"PNG");
            size_t _size = SizeofResource(module, hResource);
            auto hMemory = LoadResource(module, hResource);
            LPVOID ptr = LockResource(hMemory);

            // copy image bytes into a real hglobal memory handle
            hMemory = ::GlobalAlloc(GHND, _size);
            if (hMemory)
            {
                void* pBuffer = ::GlobalLock(hMemory);
                memcpy(pBuffer, ptr, _size);
            }

            // Create stream
            IStream* pStream = nullptr;
            HRESULT hr = CreateStreamOnHGlobal(hMemory, TRUE, &pStream);
            if (SUCCEEDED(hr))
            {
                Gdiplus::Bitmap* bmp = new Gdiplus::Bitmap(pStream);
                bmp->GetHICON(&retval);
                delete bmp;
                Gdiplus::GdiplusShutdown(token);
                pStream->Release();
            }

            if (hMemory)
                GlobalFree(hMemory);
        }

        return retval;
    }
#pragma warning (pop)

    // Get one of the system or user folders by it's GUID.
    generic_string getSystemFolder(GUID folderID)
    {

        PWSTR path;
        SHGetKnownFolderPath(folderID, KF_FLAG_DEFAULT, NULL, &path);

        generic_string retVal;
        retVal = path;
        return retVal;
    }

    // Converts a File Link into an actual filename
    void resolveLinkFile(generic_string& linkFilePath)
    {
        IShellLink* psl = nullptr;
        WCHAR targetFilePath[MAX_PATH];
        WIN32_FIND_DATA wfd = {};

        HRESULT hres = CoInitialize(NULL);
        if (SUCCEEDED(hres))
        {
            hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
            if (SUCCEEDED(hres))
            {
                IPersistFile* ppf = nullptr;
                hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
                if (SUCCEEDED(hres))
                {
                    // Load the shortcut. 
                    hres = ppf->Load(linkFilePath.c_str(), STGM_READ);
                    if (SUCCEEDED(hres) && hres != S_FALSE)
                    {
                        // Resolve the link. 
                        hres = psl->Resolve(NULL, 0);
                        if (SUCCEEDED(hres) && hres != S_FALSE)
                        {
                            // Get the path to the link target. 
                            hres = psl->GetPath(targetFilePath, MAX_PATH, (WIN32_FIND_DATA*)&wfd, SLGP_SHORTPATH);
                            if (SUCCEEDED(hres) && hres != S_FALSE)
                            {
                                linkFilePath = targetFilePath;
                            }
                        }
                    }
                    ppf->Release();
                }
                psl->Release();
            }
            CoUninitialize();
        }
    }

    // Checks if directory exist
    bool isValidDirectory(const TCHAR* sPath)
    {
        WIN32_FILE_ATTRIBUTE_DATA attributes = {};

        if (!PathFileExists(sPath))
            return false;

        if (!GetFileAttributesEx(sPath, GetFileExInfoStandard, &attributes))
            return false;

        if (!(attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            return false;

        return true;
    }

    // Checks if directory exist
    bool isValidDirectoryS(const generic_string& sPath)
    {
        return isValidDirectory(sPath.c_str());
    }

    // Returns the proper non back-slash ended directory name (eg: C:\Windows\ -> C:\Windows)
    // Needed to canonicalize directory names, since then functions of NscLib will return names with or
    // without ending backslashes.
    std::string properDirNameA(const std::string& dirName) {
        if (dirName.empty())
            return "";
        std::string retval = dirName;
        if (retval.back() == '\\')
            retval.pop_back();
        return retval;
    }

    // Returns the proper non back-slash ended directory name (eg: C:\Windows\ -> C:\Windows)
    // Needed to canonicalize directory names, since then functions of NscLib will return names with or
    // without ending backslashes.
    std::wstring properDirNameW(const std::wstring& dirName) {
        if (dirName.empty())
            return L"";
        std::wstring retval = dirName;
        if (retval.back() == TEXT('\\'))
            retval.pop_back();
        return retval;
    }

    // Try to retrieve the Neverwinter's Home path
    generic_string getNwnHomePath(int CompilerVersion)
    {
        generic_string HomePath;

        if (CompilerVersion >= 174)
        {
            TCHAR DocumentsPath[MAX_PATH];

            if (!SHGetSpecialFolderPath(NULL, DocumentsPath, CSIDL_PERSONAL, TRUE))
                return HomePath;

            HomePath = DocumentsPath;
            HomePath.append(TEXT("\\Neverwinter Nights\\"));
        }
        else {
            HKEY Key;
            LONG Status;

            Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\BioWare\\NWN\\Neverwinter"), REG_OPTION_RESERVED,
#ifdef _WIN64
                KEY_QUERY_VALUE | KEY_WOW64_32KEY,
#else
                KEY_QUERY_VALUE,
#endif
                & Key);

            if (Status != NO_ERROR)
                return HomePath;

            try
            {
                char NameBuffer[MAX_PATH + 1] = {};
                DWORD NameBufferSize;
                bool FoundIt;
                static const TCHAR* ValueNames[] =
                {
                    TEXT("Path"),     // Retail NWN2
                    TEXT("Location"), // Steam NWN2
                };
                FoundIt = false;

                for (size_t i = 0; i < _countof(ValueNames); i += 1)
                {
                    NameBufferSize = sizeof(NameBuffer) - sizeof(NameBuffer[0]);
                    Status = RegQueryValueEx(Key, ValueNames[i], NULL, NULL, (LPBYTE)NameBuffer, &NameBufferSize);

                    if (Status != NO_ERROR)
                        continue;

                    if ((NameBufferSize > 0) &&
                        (NameBuffer[NameBufferSize - 1] == '\0'))
                        NameBufferSize -= 1;

                    HomePath = str2wstr(NameBuffer);
                }
            }
            catch (...)
            {
                RegCloseKey(Key);
            }
        }

        return HomePath;
    }

    // Checks for a path's write permission.
    // returns false if not successful (eg: file/path doesn't exist), else returns true and fills outFilePermission enums
    bool checkWritePermission(const generic_string& sPath, PathWritePermission& outPathPermission)
    {
        WIN32_FILE_ATTRIBUTE_DATA attributes = {};

        if (!PathFileExists(sPath.c_str()))
            return false;

        if (!GetFileAttributesEx(sPath.c_str(), GetFileExInfoStandard, &attributes))
            return false;

        if (attributes.dwFileAttributes & FILE_ATTRIBUTE_READONLY)
        {
            outPathPermission = PathWritePermission::FileIsReadOnly;
            return true;
        }

        // On directories we try to write a temporary file to test permissions.
        if (attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            TCHAR buffer[MAX_PATH];
            UINT retval = GetTempFileName(sPath.c_str(), TEXT("nws"), 0, buffer);
            if (retval == 0)
            {
                outPathPermission = PathWritePermission::RequiresAdminPrivileges;
                return true;
            }

            DeleteFile(buffer);
            outPathPermission = PathWritePermission::WriteAllowed;
            return true;
        }

        // We are only touching a file, hence all shared attributes may apply
        // Also, to Open a file, we call a "CreateFile" function. Lol Windows API
        HANDLE f = CreateFile(sPath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL, OPEN_EXISTING, attributes.dwFileAttributes, NULL);

        // The GetLastError codes are documented here:
        // https://docs.microsoft.com/en-us/windows/win32/debug/system-error-codes
        if (f == INVALID_HANDLE_VALUE)
        {
            switch (GetLastError())
            {
            case ERROR_ACCESS_DENIED:
                outPathPermission = PathWritePermission::RequiresAdminPrivileges;
                break;
            case ERROR_SHARING_VIOLATION:
                outPathPermission = PathWritePermission::BlockedByApplication;
                break;
            default:
                outPathPermission = PathWritePermission::UndeterminedError;
            }

            return true;
        }

        CloseHandle(f);
        outPathPermission = PathWritePermission::WriteAllowed;
        return true;
    }

    // Loads a raw file into a string buffer
    bool fileToBuffer(const generic_string& filePath, std::string& sContents)
    {
        std::ifstream fileReadStream;

        fileReadStream.open(filePath.c_str(), std::ios::in | std::ios::binary);
        if (!fileReadStream.is_open())
        {
            return false;
        }

        fileReadStream.seekg(0, std::ios::end);
        sContents.resize(static_cast<std::size_t>(fileReadStream.tellg()));
        fileReadStream.seekg(0, std::ios::beg);
        size_t fileSize = sContents.size();
        fileReadStream.read(&sContents[0], fileSize);
        fileReadStream.close();

        return true;
    }

    // Saves a string buffer into a raw file 
    bool bufferToFile(const generic_string& filePath, const std::string& sContents)
    {
        std::ofstream s;
        s.open(filePath.c_str(), std::ios::out | std::ios::binary | std::ios::trunc);
        if (!s.is_open())
        {
            return false;
        }

        s << sContents;
        s.close();
        return true;
    }

    // Writes a pseudo-batch file to store Notepad++ executable to be called by ShellExecute
    // with a delay (So it can restart seeamlessly).
    bool writePseudoBatchExecute(const generic_string& path, const generic_string& executePath)
    {
        std::stringstream stext;

        // Use ping trick on batch to delay execution, since it's the most compatible option to add a delay
        stext << "@echo off" << std::endl;
        stext << "ping -n 1 -w 1000 10.255.255.255 > nul" << std::endl;
        stext << "\"" << wstr2str(executePath.c_str()) << "\"" << std::endl;

        return bufferToFile(path, stext.str());
    }

    // ShellExecutes a given path with elevated privileges or not.
    void runProcess(bool isElevationRequired, const generic_string& path, const generic_string& args,
        const generic_string& opendir)
    {
        const TCHAR* opVerb = isElevationRequired ? TEXT("runas") : TEXT("open");
        ::ShellExecute(NULL, opVerb, path.c_str(), args.c_str(), opendir.c_str(), SW_HIDE);
    }

    // Returns a temporary filename string
    bool getTemporaryFile(const generic_string& path, generic_string& outputPath)
    {
        TCHAR buffer[MAX_PATH + 1];
        if (GetTempFileName(path.c_str(), TEXT("nwsplugin"), 0, buffer))
        {
            outputPath = buffer;
            return true;
        }

        return false;
    }

    // Recursively navigate on XML nodes and get rid of all comment tags.
    // Bonus: also deletes the declaration headers now, since we need to rebuild it.
    void stripXMLInfo(tinyxml2::XMLNode* node)
    {
        // All XML nodes may have children and siblings. So for each valid node, first we
        // iterate on it's (possible) children, and then we proceed to clear the node itself and jump 
        // to the next sibling
        while (node)
        {
            if (node->FirstChild() != NULL)
                stripXMLInfo(node->FirstChild());

            //Check to see if current node is a comment
            auto comment = dynamic_cast<tinyxml2::XMLComment*>(node);
            if (comment)
            {
                // If it is, we ask the parent to delete this, but first move pointer to next member so we don't get lost in a NULL reference
                node = node->NextSibling();
                comment->Parent()->DeleteChild(comment);
            }
            // We also remove XML declarations here
            else
            {
                auto declare = dynamic_cast<tinyxml2::XMLDeclaration*>(node);
                if (declare)
                {
                    node = node->NextSibling();
                    declare->Parent()->DeleteChild(declare);
                }
                else
                    node = node->NextSibling();
            }
        }
    }

    // Recursively navigate the XML structure until a determined first occurrence of an element is found.
    // (optional) Also checks if element has attribute checkAttribute with corresponding checkAttributeValue.
    // Returns NULL on a failed search.
    tinyxml2::XMLElement* searchElement(tinyxml2::XMLElement* const from, const std::string& toName,
        const std::string checkAttribute, const std::string checkAttributeValue)
    {
        tinyxml2::XMLElement* _from = from;
        tinyxml2::XMLElement* found = NULL;

        while (_from && !found)
        {
            if (_from->Name() == toName)
            {
                if (checkAttribute.empty())
                {
                    found = _from;
                    break;
                }
                else
                {
                    if (_from->Attribute(checkAttribute.c_str(), checkAttributeValue.c_str()))
                    {
                        found = _from;
                        break;
                    }
                }
            }

            if (_from->FirstChildElement() != NULL)
                found = searchElement(_from->FirstChildElement(), toName, checkAttribute, checkAttributeValue);

            if (!found)
                _from = _from->NextSiblingElement();
        }

        return found;

    };

    // Adapted from function source:
    // https://www.codeguru.com/cplusplus/finding-a-menuitem-from-command-id/
    HMENU FindSubMenu(HMENU baseMenu, generic_string menuName)
    {
        int myPos;
        if (baseMenu == NULL)
            // Sorry, Wrong Number
            return NULL;

        int count = GetMenuItemCount(baseMenu);
        HMENU returnValue = NULL;
        for (myPos = 0; myPos < count && returnValue == NULL; myPos++)
        {
            // Is this the real one?
            generic_string toCompare = GetMenuItemName(baseMenu, myPos);
            if (GetMenuItemName(baseMenu, myPos) == menuName)
                // Yep!
                return GetSubMenu(baseMenu, myPos);

            // Maybe in a subMenu?
            HMENU mNewMenu = GetSubMenu(baseMenu, myPos);
            if (mNewMenu != NULL)
                // rekursive!
                returnValue = FindSubMenu(mNewMenu, menuName);

            // If not found... next!!!
        }

        return returnValue; // iterate in the upper stackframe
    }

    // Returns the menu name for a given position inside a menu handle
    generic_string GetMenuItemName(HMENU menu, int position)
    {
        MENUITEMINFO menuInfo = {};
        TCHAR szString[256] = {};

        ZeroMemory(szString, sizeof(szString));
        menuInfo.cch = 256;
        menuInfo.fMask = MIIM_TYPE;
        menuInfo.fType = MFT_STRING;
        menuInfo.cbSize = sizeof(MENUITEMINFO);
        menuInfo.dwTypeData = szString;
        bool bSuccess = GetMenuItemInfo(menu, position, MF_BYPOSITION, &menuInfo);

        if (bSuccess)
            return generic_string(szString);
        else
            return generic_string(TEXT(""));
    }

    // Creates a file list from a starting directory with recurse options
    // Sampled from: 
    // https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
    // This version have a cancel flag for assynchronous interruptions
    void createFilesList(std::vector<fs::path>& filesList, const generic_string& startingDir,
        const generic_string& fileMask, bool recurse, std::atomic<bool>& cancelFlagVariable)
    {

        WIN32_FIND_DATA findData;

        generic_string searchKey = startingDir + TEXT("\\") + fileMask;
        HANDLE h = FindFirstFile(searchKey.c_str(), &findData);
        if (h == INVALID_HANDLE_VALUE)
            return;

        while (true)
        {
            filesList.push_back(startingDir + TEXT("\\") + findData.cFileName);
            if (!FindNextFile(h, &findData))
                break;

            if (cancelFlagVariable)
                return;
        }

        if (recurse)
        {
            for (const auto& entry : fs::directory_iterator(startingDir))
            {
                if (cancelFlagVariable)
                    return;

                std::string s = entry.path().string().c_str();
                std::string t = entry.path().filename().string().c_str();

                if (entry.is_directory() && entry.path().filename().string() != "." && entry.path().filename().string() != "..")
                    createFilesList(filesList, properDirNameW(entry.path().c_str()), fileMask, true, cancelFlagVariable);
            }
        }
    }

    // Folder browsing function callback
    // For changing default selected directory
    // Extracted from: 
    // https://stackoverflow.com/questions/6942150/why-folderbrowserdialog-dialog-does-not-scroll-to-selected-folder
    int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
    {
        LPITEMIDLIST pidlNavigate;
        switch (uMsg)
        {
            case BFFM_INITIALIZED:
            {
                pidlNavigate = (LPITEMIDLIST)lpData;
                if (pidlNavigate != NULL)
                    SendMessage(hwnd, BFFM_SETSELECTION, (WPARAM)TRUE, (LPARAM)pidlNavigate);

                break;
            }

            case BFFM_SELCHANGED:
            {
                EnumChildWindows(hwnd, EnumCallback, 0);
                break;
            }
        }
        return 0;
    }

    // Folder browsing child enumeration callback
    // For changing default selected directory
    // Extracted from: 
    // https://stackoverflow.com/questions/6942150/why-folderbrowserdialog-dialog-does-not-scroll-to-selected-folder
    BOOL CALLBACK EnumCallback(HWND hWndChild, LPARAM lParam)
    {
        TCHAR szClass[MAX_PATH];
        ZeroMemory(szClass, std::size(szClass) * sizeof(TCHAR));
        HTREEITEM hNode;
#pragma warning(push)
#pragma warning(disable : 6386)
        if (GetClassName(hWndChild, szClass, sizeof(szClass))
            && _tcscmp(szClass, TEXT("SysTreeView32")) == 0) 
        {
#pragma warning(pop)
            hNode = TreeView_GetSelection(hWndChild);
            TreeView_EnsureVisible(hWndChild, hNode);
            return FALSE;
        }
        return TRUE;       
    }

#pragma warning(pop)

}
