/*++

Copyright (c) Ken Johnson (Skywing). All rights reserved.

Module Name:

    Main.cpp

Abstract:

    This module houses the main entry point of the compiler driver.  The
    compiler driver provides a user interface to compile scripts under user
    control.

--*/
#define _WINDOWS
#ifdef _WINDOWS
#include <time.h>
#include <io.h>
#include <Shlobj.h>
#include <comutil.h> //for _bstr_t (used in the string conversion)
#define strtok_r strtok_s
//#define access    _access_s
#pragma comment(lib, "comsuppw")
#endif

#include <vector>
#include <list>
#include <fstream>
#include <iostream>
#include "Nsc.h"
#include "findfirst.h"
#include "version.h"
#include "JSON.h"

#if defined(__linux__)
#include <unistd.h>
#include <stdarg.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <libgen.h>
#include <chrono>
#include <pwd.h>
#endif

#if defined(__APPLE__)
//#include <codecvt>
#endif

#include "easylogging++.h"

INITIALIZE_EASYLOGGINGPP

typedef std::vector<std::string> StringVec;
typedef std::vector<const char*> StringArgVec;

std::vector<std::pair<std::string, std::string>> NwnVersions = {
    { "00840", "NWN EE Digital Deluxe Beta (Head Start)" },
    { "00829", "NWN EE Beta (Head Start)" },
    { "00839", "NWN EE Digital Deluxe" },
    { "00785", "NWN EE" }
};

#pragma warning (disable : 4091)
typedef enum _NSCD_FLAGS {
    //
    // Stop processing files on the first error.
    //

    NscDFlag_StopOnError = 0x00000001,

    NscDFlag_LastFlag
};


FILE* g_Log;

//
// Define the debug text output interface, used to write debug or log messages
// to the user.
//

class PrintfTextOut : public IDebugTextOut {

public:

    inline
        PrintfTextOut() {}

    inline
        ~PrintfTextOut() {}

    inline
        virtual
        void
        WriteText(
            const char* fmt, ...) {
        va_list ap;

        va_start(ap, fmt);
        WriteTextV(fmt, ap);
        va_end(ap);
    }

    inline
        virtual
        void
        WriteTextV(
            const char* fmt,
            va_list ap
        )
        /*++

        Routine Description:

            This routine displays text to the log file and the debug console.

            The console output may have color attributes supplied, as per the standard
            SetConsoleTextAttribute API.

            Additionally, if a log file has been opened, a timestamped log entry is
            written to disk.

        Arguments:

            Attributes - Supplies color attributes for the text as per the standard
                         SetConsoleTextAttribute API (e.g. FOREGROUND_RED).

            fmt - Supplies the g_TextOut.WriteText-style format string to use to display text.

            argptr - Supplies format inserts.

        Return Value:

            None.

        Environment:

            User mode.

        --*/
    {
        char buf[8193];

        vsnprintf(buf, sizeof(buf), fmt, ap);

        puts(buf);

        if (g_Log != nullptr) {
            time_t t;
            struct tm* tm;

            time(&t);

            if ((tm = gmtime(&t)) != nullptr) {
                fprintf(
                    g_Log,
                    "[%04lu-%02lu-%02lu %02lu:%02lu:%02lu] ",
                    tm->tm_year + 1900,
                    tm->tm_mon + 1,
                    tm->tm_mday,
                    tm->tm_hour,
                    tm->tm_min,
                    tm->tm_sec);
            }

            vfprintf(g_Log, fmt, ap);
            fflush(g_Log);
        }
    }

};


//
// No reason these should be globals, except for ease of access to the debugger
// right now.
//

PrintfTextOut g_TextOut;
ResourceManager* g_ResMan;

std::string ws2s(const std::wstring& wstr)
{
    if (wstr.empty())
    {
        return "";
    }

    const auto size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr.at(0), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (size_needed < 1)
    {
        throw std::runtime_error("WideCharToMultiByte() failed: " + std::to_string(size_needed));
    }

    std::string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr.at(0), (int)wstr.size(), &result.at(0), size_needed, nullptr, nullptr);
    return result;
}

bool FileExists(const std::string& Filename)
{
    return access(Filename.c_str(), 0) == 0;
}

std::string GetHomeDirectory() {
#if defined(__linux__) || defined(__APPLE__)
    char* homedir = getenv("HOME");
    if (homedir == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    LOG(DEBUG) << "HomeDir " << homedir;
    return std::string(homedir);
#elif defined(_WINDOWS)
    PWSTR wszPath;
    SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, NULL, &wszPath);

    _bstr_t bstrPath(wszPath);
    std::string strPath((char*)bstrPath);

    LOG(DEBUG) << "HomeDir " << strPath;
    return std::string(strPath);
#endif
}

std::string
GetNwnInstallPath(int CompilerVersion, bool Quiet)
/*++

Routine Description:

    This routine attempts to auto detect the NWN1 installation path from the
    registry.

Arguments:

    None.

Return Value:

    The routine returns the game installation path if successful.  Otherwise,
    an std::exception is raised.

Environment:

    User mode.

--*/
{
    std::string RootDir;
    char* EnvNwnRoot;
    std::string KeyFile;

    EnvNwnRoot = getenv("NWN_ROOT");

    if (EnvNwnRoot != NULL) {
        RootDir = EnvNwnRoot;
#if defined(_WINDOWS)
        if (RootDir.back() != '\\')
            RootDir.push_back('\\');
        KeyFile = "\\data\\nwn_base.key";
#else
        if (RootDir.back() != '/')
            RootDir.push_back('/');
        KeyFile = "/data/nwn_base.key";
#endif
        if (!Quiet) {
            g_TextOut.WriteText("Base game location from NWN_ROOT - %s\n", RootDir.c_str());
        }
        return RootDir;
    }
    else {
        if (CompilerVersion >= 174) {

            std::string settingsFile;

#if defined(__APPLE__)
            settingsFile = GetHomeDirectory() + "/Library/Application Support/Beamdog Client/settings.json";
#elif defined(__linux__)
            settingsFile = GetHomeDirectory() + "/.config/Beamdog Client/settings.json";
#elif defined(_WINDOWS)
            settingsFile = GetHomeDirectory() + "\\Beamdog Client\\settings.json";
#endif
            LOG(DEBUG) << " settingsFile " << settingsFile;

            if (FileExists(settingsFile)) {
                std::ifstream InStream(settingsFile, std::ifstream::in);
                std::stringstream buffer;
                buffer << InStream.rdbuf();

                std::string str(buffer.str());

                // Parse data
                JSONValue* value = JSON::Parse(str.c_str());

                if (value != NULL) {

                    JSONObject root;
                    if (value->IsObject() != false) {
                        root = value->AsObject();

                        // Retrieving an array
                        if (root.find(L"folders") != root.end() && root[L"folders"]->IsArray()) {
                            JSONArray array = root[L"folders"]->AsArray();

                            if (array.size() > 0) {
                                RootDir = ws2s(array[0]->AsString());
                            }

#ifdef _WINDOWS
                            RootDir.push_back('\\');
#else
                            RootDir.push_back('/');
#endif

                            std::string GameStr;

                            std::vector<std::pair<std::string, std::string>>::iterator it;  // declare an iterator to a vector of strings
                            for (it = NwnVersions.begin(); it != NwnVersions.end(); it++) {
                                if (FileExists(RootDir + it->first + KeyFile)) {
                                    RootDir += it->first;
#ifdef _WINDOWS
                                    RootDir.push_back('\\');
#else
                                    RootDir.push_back('/');
#endif
                                    GameStr = it->second;
                                    break;
                                }
                            }

                            LOG(DEBUG) << " RootDir " << RootDir;
                            if (!Quiet) {
                                g_TextOut.WriteText("Base game %s location - %s\n", GameStr.c_str(), RootDir.c_str());
                            }
                        }

                    }
                }
            }
            else {
                std::string SteamRootDir;
#if defined(__APPLE__)
                SteamRootDir = GetHomeDirectory() + "/Library/Application Support/Steam/steamapps/common/Neverwinter Nights/";
#elif defined(__linux__)
                SteamRootDir = GetHomeDirectory() + "/.local/share/Steam/steamapps/common/Neverwinter Nights/";
#elif defined(_WINDOWS)
                SteamRootDir = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Neverwinter Nights\\";
#endif
                if (FileExists(SteamRootDir + KeyFile))
                    RootDir = SteamRootDir;
            }
            return RootDir;

        }
        else {
#if defined(_WINDOWS)
            HKEY Key;
            LONG Status;

            Status = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                TEXT("SOFTWARE\\BioWare\\NWN\\Neverwinter"),
                REG_OPTION_RESERVED,
#ifdef _WIN64
                KEY_QUERY_VALUE | KEY_WOW64_32KEY,
#else
                KEY_QUERY_VALUE,
#endif
                & Key);

            if (Status != NO_ERROR)
                throw std::runtime_error("Unable to open NWN registry key");

            try
            {
                CHAR                NameBuffer[MAX_PATH + 1];
                DWORD               NameBufferSize;
                bool                FoundIt;
                static const char* ValueNames[] =
                {
                    "Path",     // Retail NWN
                    "Location", // Steam NWN
                };

                FoundIt = false;

                for (size_t i = 0; i < _countof(ValueNames); i += 1)
                {
                    NameBufferSize = sizeof(NameBuffer) - sizeof(NameBuffer[0]);

                    Status = RegQueryValueExA(
                        Key,
                        ValueNames[i],
                        nullptr,
                        nullptr,
                        (LPBYTE)NameBuffer,
                        &NameBufferSize);

                    if (Status != NO_ERROR)
                        continue;

                    //
                    // Strip trailing nullptr byte if it exists.
                    //

                    if ((NameBufferSize > 0) &&
                        (NameBuffer[NameBufferSize - 1] == '\0'))
                        NameBufferSize -= 1;

                    std::string NwnRootDir = std::string(NameBuffer, NameBufferSize);

                    if (!Quiet) {
                        g_TextOut.WriteText("Base game location - %s\n", NwnRootDir.c_str());
                    }

                    return NwnRootDir;
                }

                throw std::exception("Unable to read Path from NWN registry key");
            }
            catch (...)
            {
                RegCloseKey(Key);
                throw;
            }
#else
            return "";
#endif
        }
    }
}

std::string
GetNwnHomePath(int CompilerVersion, bool Quiet)
/*++

Routine Description:

    This routine attempts to auto detect the NWN home directory path from the
    current user environment.  The home path is where per-user data, such as
    most module data, HAK files, the server vault, etc are stored.

Arguments:

    None.

Return Value:

    The routine returns the game per-user home path if successful.  Otherwise,
    an std::exception is raised.

Environment:

    User mode.

--*/
{
    std::string HomePath;

    if (CompilerVersion >= 174) {

#if defined(__APPLE__)
        HomePath = "~/Documents/Neverwinter Nights/";
#elif defined(__linux__)
        HomePath = "~/Documents/Neverwinter Nights/";
#elif defined(_WINDOWS)
        CHAR        DocumentsPath[MAX_PATH];

        if (!SHGetSpecialFolderPathA(NULL, DocumentsPath, CSIDL_PERSONAL, TRUE))
            throw std::runtime_error("Couldn't get user documents path.");

        HomePath = DocumentsPath;
        HomePath += "\\Neverwinter Nights\\";
#else
        HomePath = ""
#endif
    }
    else {
#if defined(_WINDOWS)
        HKEY Key;
        LONG Status;

        Status = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("SOFTWARE\\BioWare\\NWN\\Neverwinter"),
            REG_OPTION_RESERVED,
#ifdef _WIN64
            KEY_QUERY_VALUE | KEY_WOW64_32KEY,
#else
            KEY_QUERY_VALUE,
#endif
            & Key);

        if (Status != NO_ERROR)
            throw std::runtime_error("Unable to open NWN registry key");

        try
        {
            CHAR                NameBuffer[MAX_PATH + 1];
            DWORD               NameBufferSize;
            bool                FoundIt;
            static const char* ValueNames[] =
            {
                "Path",     // Retail NWN2
                "Location", // Steam NWN2
            };

            FoundIt = false;

            for (size_t i = 0; i < _countof(ValueNames); i += 1)
            {
                NameBufferSize = sizeof(NameBuffer) - sizeof(NameBuffer[0]);

                Status = RegQueryValueExA(
                    Key,
                    ValueNames[i],
                    NULL,
                    NULL,
                    (LPBYTE)NameBuffer,
                    &NameBufferSize);

                if (Status != NO_ERROR)
                    continue;

                //
                // Strip trailing null byte if it exists.
                //

                if ((NameBufferSize > 0) &&
                    (NameBuffer[NameBufferSize - 1] == '\0'))
                    NameBufferSize -= 1;

                HomePath = std::string(NameBuffer, NameBufferSize);
            }

            throw std::exception("Unable to read Path from NWN registry key");
        }
        catch (...)
        {
            RegCloseKey(Key);
            throw;
        }
#else
        HomePath = "";
#endif
    }
    LOG(DEBUG) << " HomePath " << HomePath;
    if (!Quiet)
    {
        g_TextOut.WriteText("Home Path - %s\n", HomePath.c_str());
    }


    return HomePath;
}


void
LoadScriptResources(
    ResourceManager& ResMan,
    const std::string& NWNHome,
    const std::string& InstallDir,
    bool Erf16,
    int Compilerversion
)
/*++

Routine Description:

    This routine loads the game script data into the resource system.

Arguments:

    ResMan - Supplies the ResourceManager instance that is to load the module.

    NWNHome - Supplies the users NWN2 home directory (i.e. NWN2 Documents dir).

    InstallDir - Supplies the game installation directory.

    Erf16 - Supplies a Boolean value indicating true if 16-byte ERFs are to be
            used (i.e. for NWN1-style modules), else false if 32-byte ERFs are
            to be used (i.e. for NWN2-style modules).

    CustomModPath - Optionally supplies an override path to search for a module
                    file within, bypassing the standard module load heuristics.

Return Value:

    None.  On failure, an std::exception is raised.

Environment:

    User mode.

--*/
{
    ResourceManager::ModuleLoadParams LoadParams;
    ResourceManager::StringVec KeyFiles;

    ZeroMemory(&LoadParams, sizeof(LoadParams));

    LoadParams.SearchOrder = ResourceManager::ModSearch_PrefDirectory;
    LoadParams.ResManFlags = ResourceManager::ResManFlagNoGranny2;

    LoadParams.ResManFlags |= ResourceManager::ResManFlagErf16;

    if (Compilerversion >= 174) {
#ifdef _WINDOWS
        KeyFiles.push_back("data\\nwn_base");
#else
        KeyFiles.emplace_back("data/nwn_base");
#endif // _WINDOWS
    }
    else {
        KeyFiles.emplace_back("xp3");
        KeyFiles.emplace_back("xp2patch");
        KeyFiles.emplace_back("xp2");
        //KeyFiles.emplace_back("xp1patch");
        KeyFiles.emplace_back("xp1");
        KeyFiles.emplace_back("chitin");
    }

    LoadParams.KeyFiles = &KeyFiles;

    LoadParams.ResManFlags |= ResourceManager::ResManFlagBaseResourcesOnly;

    ResMan.LoadScriptResources(
        NWNHome,
        InstallDir,
        &LoadParams
    );
}


bool
LoadFileFromDisk(
    const std::string& FileName,
    std::vector<unsigned char>& FileContents
)
/*++

Routine Description:

    This routine loads a file from a raw disk
    This routine canonicalizes an input file name to its resource name and
    resource type, and then loads the entire file contents into memory.

    The input file may be a short filename or a filename with a path.  It may be
    backed by the raw filesystem or by the resource system (in that order of
    precedence).

Arguments:

    ResMan - Supplies the resource manager to use to service file load requests.

    InFile - Supplies the filename of the file to load.

    FileResRef - Receives the canonical RESREF name of the input file.

    FileResType - Receives the canonical ResType (extension) of the input file.

    FileContents - Receives the contents of the input file.

Return Value:

    The routine returns a Boolean value indicating true on success, else false
    on failure.

Environment:

    User mode.

--*/
{
    FileWrapper FileWrap;
    HANDLE SrcFile;

#if defined(_WINDOWS)


    FileContents.clear();


    SrcFile = CreateFileA(
        FileName.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);


    if (SrcFile == nullptr)
        return false;

    try
    {
        FileWrap.SetFileHandle(SrcFile, true);

        if ((size_t)FileWrap.GetFileSize() != 0)
        {
            FileContents.resize((size_t)FileWrap.GetFileSize());

            FileWrap.ReadFile(
                &FileContents[0],
                FileContents.size(),
                "LoadFileFromDisk File Contents");
        }
    }
    catch (std::exception)
    {
        CloseHandle(SrcFile);
        SrcFile = nullptr;
        return false;
    }

    CloseHandle(SrcFile);
    SrcFile = nullptr;

    return true;
#else

    SrcFile = fopen(FileName.c_str(), "r");

    if (SrcFile == NULL) {
        LOG(DEBUG) << "Failed to Open Script " << FileName.c_str();
        return false;
    }

    FileContents.clear();

    try {
        FileWrap.SetFileHandle(SrcFile, true);
        if ((size_t)FileWrap.GetFileSize() != 0) {
            FileContents.resize((size_t)FileWrap.GetFileSize());

            FileWrap.ReadFile(
                &FileContents[0],
                FileContents.size(),
                "LoadFileFromDisk File Contents");
        }

    }
    catch (std::exception& ex) {
        LOG(DEBUG) << "Error reading script file " << FileName.c_str();
        LOG(DEBUG) << ex.what();
        fclose(SrcFile);
        SrcFile = nullptr;
        return false;
    }

    fclose(SrcFile);
    return true;


#endif
}

bool
LoadInputFile(
    ResourceManager& ResMan,
    IDebugTextOut* TextOut,
    const std::string& InFile,
    NWN::ResRef32& FileResRef,
    NWN::ResType& FileResType,
    std::vector<unsigned char>& FileContents
)
/*++

Routine Description:

    This routine canonicalizes an input file name to its resource name and
    resource type, and then loads the entire file contents into memory.

    The input file may be a short filename or a filename with a path.  It may be
    backed by the raw filesystem or by the resource system (in that order of
    precedence).

Arguments:

    ResMan - Supplies the resource manager to use to service file load requests.

    TextOut - Supplies the text out interface used to receive any diagnostics
              issued.

    InFile - Supplies the filename of the file to load.

    FileResRef - Receives the canonical RESREF name of the input file.

    FileResType - Receives the canonical ResType (extension) of the input file.

    FileContents - Receives the contents of the input file.

Return Value:

    The routine returns a Boolean value indicating true on success, else false
    on failure.

    On catastrophic failure, an std::exception is raised.

Environment:

    User mode.

--*/
{

    //
    // First, canonicalize the filename.
    //

#if defined(_WINDOWS)
    char                   Drive[_MAX_DRIVE];
    char                   Dir[_MAX_DIR];
    char                   FileName[_MAX_FNAME];
    char                   Extension[_MAX_EXT];

    if (_splitpath_s(
        InFile.c_str(),
        Drive,
        Dir,
        FileName,
        Extension))
    {
        TextOut->WriteText(
            "Error: Malformed file pathname \"%s\".\n", InFile.c_str());

        return false;
    }
#else
    char* Dir;
    char* FileName;
    std::string Extension;

    char dirc[_MAX_DIR];
    char filec[_MAX_FNAME];

    strncpy(dirc, InFile.c_str(), _MAX_DIR);
    strncpy(filec, InFile.c_str(), _MAX_FNAME);

    Dir = dirname(dirc);
    FileName = basename(filec);
    FileName = OsCompat::filename(FileName);
    Extension = OsCompat::getFileExt(InFile.c_str());

#endif

#if defined(_WINDOWS)
    FileResType = ResMan.ExtToResType(Extension + 1);
#else
    FileResType = ResMan.ExtToResType(Extension.c_str());
#endif

    FileResRef = ResMan.ResRef32FromStr(FileName);

    //
    // Load the file directly if we can, otherwise attempt it via the resource
    // system.
    //

    if (!access(InFile.c_str(), 0)) {
        return LoadFileFromDisk(InFile, FileContents);
    }
    else {
        LOG(DEBUG) << "Script failed existence check " << InFile.c_str();
        return false;
    }
}

bool
CompileSourceFile(
    NscCompiler& Compiler,
    int CompilerVersion,
    bool Optimize,
    bool IgnoreIncludes,
    bool SuppressDebugSymbols,
    bool Quiet,
    bool VerifyCode,
    IDebugTextOut* TextOut,
    UINT32 CompilerFlags,
    const NWN::ResRef32 InFile,
    const std::vector<unsigned char>& InFileContents,
    const std::string& OutBaseFile
)
/*++

Routine Description:

    This routine compiles a single source file according to the specified set of
    compilation options.

Arguments:

    NscCompiler - Supplies the compiler context that will be used to process the
                  request.

    CompilerVersion - Supplies the BioWare-compatible compiler version number.

    Optimize - Supplies a Boolean value indicating true if the script should be
               optimized.

    IgnoreIncludes - Supplies a Boolean value indicating true if include-only
                     source files should be ignored.

    SuppressDebugSymbols - Supplies a Boolean value indicating true if debug
                           symbol generation should be suppressed.

    Quiet - Supplies a Boolean value that indicates true if non-critical
            messages should be silenced.

    VerifyCode - Supplies a Boolean value that indicates true if generated code
                 is to be verified with the analyzer/verifier if compilation was
                 successful.

    TextOut - Supplies the text out interface used to receive any diagnostics
              issued.

    CompilerFlags - Supplies compiler control flags.  Legal values are drawn
                    from the NscCompilerFlags enumeration.

    InFile - Supplies the RESREF corresponding to the input file name.

    InFileContents - Supplies the contents of the input file.

    OutBaseFile - Supplies the base name (potentially including path) of the
                  output file.  No extension is present.

Return Value:

    The routine returns a Boolean value indicating true on success, else false
    on failure.

    On catastrophic failure, an std::exception is raised.

Environment:

    User mode.

--*/
{
    std::vector<unsigned char> Code;
    std::vector<unsigned char> Symbols;
    std::set<std::string> Dependencies;
    NscResult Result;
    std::string FileName;
    FILE* f;

    char filec[_MAX_FNAME];

    strncpy(filec, InFile.RefStr, _MAX_FNAME);

    if (!Quiet) {
        TextOut->WriteText("Compiling: %s.nss", InFile.RefStr);
    }

    //
    // Execute the main compilation pass.
    //

    Result = Compiler.NscCompileScript(
        InFile,
        (!InFileContents.empty()) ? &InFileContents[0] : nullptr,
        InFileContents.size(),
        CompilerVersion,
        Optimize,
        IgnoreIncludes,
        TextOut,
        CompilerFlags,
        Code,
        Symbols,
        Dependencies);

    switch (Result) {

    case NscResult_Failure:
        TextOut->WriteText(
            "Compilation aborted with errors.\n");

        return false;

    case NscResult_Include:
        if (!Quiet) {
            TextOut->WriteText(
                "%s.nss is an include file, ignored.",
                InFile.RefStr);
        }

        return true;

    case NscResult_Success:
        break;

    default:
        TextOut->WriteText(
            "Unknown compiler status code.\n");

        return false;

    }

    //
    // If we compiled successfully, write the results to disk.
    //

    FileName = OutBaseFile;
    FileName += ".ncs";

    f = fopen(FileName.c_str(), "wb");

    if (f == nullptr) {
        LOG(DEBUG) << "Error Number " << errno;
        TextOut->WriteText(
            "Error: Unable to open output file %s.\n",
            FileName.c_str());

        return false;
    }

    if (!Code.empty()) {
        if (fwrite(&Code[0], Code.size(), 1, f) != 1) {
            fclose(f);

            TextOut->WriteText(
                "Error: Failed to write to output file %s.\n",
                FileName.c_str());

            return false;
        }
    }

    fclose(f);

    if (!SuppressDebugSymbols) {
        FileName = OutBaseFile;
        FileName += ".ndb";

        f = fopen(FileName.c_str(), "wb");

        if (f == nullptr) {
            TextOut->WriteText(
                "Error: Failed to open debug symbols file %s.\n",
                FileName.c_str());

            return false;
        }

        if (!Symbols.empty()) {
            if (fwrite(&Symbols[0], Symbols.size(), 1, f) != 1) {
                fclose(f);

                TextOut->WriteText(
                    "Error: Failed to write to debug symbols file %s.\n",
                    FileName.c_str());

                return false;
            }
        }

        fclose(f);
    }

    if (CompilerFlags & NscCompilerFlag_GenerateMakeDeps) {
        FileName = OutBaseFile;
        FileName += ".d";

        f = fopen(FileName.c_str(), "w");

        if (f == nullptr) {
            TextOut->WriteText(
                "Error: Failed to open dependency file %s.\n",
                FileName.c_str());

            return false;
        }

        if (!Dependencies.empty()) {
            // NCS primarily depends on the NSS
            fprintf(f, "%s.ncs: %s.nss ", OutBaseFile.c_str(), OutBaseFile.c_str());

            // Print all other dependencies
            for (auto& dep : Dependencies)
                fprintf(f, " \\\n    %s", dep.c_str());

            // Create phony rules for dependencies, so deleting them doesn't
            // require deleting all .d files as well.
            for (auto& dep : Dependencies)
                fprintf(f, "\n%s:\n", dep.c_str());
        }

        fclose(f);
    }
    return true;
}


bool
DisassembleScriptFile(
    ResourceManager& ResMan,
    NscCompiler& Compiler,
    bool Quiet,
    IDebugTextOut* TextOut,
    const NWN::ResRef32& InFile,
    const std::vector<unsigned char>& InFileContents,
    const std::vector<unsigned char>& DbgFileContents,
    const std::string& OutBaseFile
)
/*++

Routine Description:

    This routine processes a single input file according to the desired compile
    or diassemble options.

Arguments:

    ResMan - Supplies the resource manager to use to service file load requests.

    NscCompiler - Supplies the compiler context that will be used to process the
                  request.

    Quiet - Supplies a Boolean value that indicates true if non-critical
            messages should be silenced.

    TextOut - Supplies the text out interface used to receive any diagnostics
              issued.

    InFile - Supplies the RESREF corresponding to the input file name.

    InFileContents - Supplies the contents of the input file.

    DbgFileContents - Supplies the contents of the associated debug symbols, if
                      any could be located.  This may be empty.

    OutBaseFile - Supplies the base name (potentially including path) of the
                  output file.  No extension is present.

Return Value:

    The routine returns a Boolean value indicating true on success, else false
    on failure.

    On catastrophic failure, an std::exception is raised.

Environment:

    User mode.

--*/
{
    std::string Disassembly;
    std::string FileName;
    std::string ScriptTempFile;
    std::string SymbolsTempFile;
    FILE* f;
    std::list<NscPrototypeDefinition> ActionPrototypes;

    if (!Quiet) {
        TextOut->WriteText(
            "Diassembling: %s\n",
            InFile.RefStr);
    }

    //
    // Disassemble the script to raw assembly.
    //

    Compiler.NscDisassembleScript(
        (!InFileContents.empty()) ? &InFileContents[0] : nullptr,
        InFileContents.size(),
        Disassembly);

    FileName = OutBaseFile;
    FileName += ".pcode";

    f = fopen(FileName.c_str(), "wt");

    if (f == nullptr) {
        TextOut->WriteText(
            "Error: Unable to open disassembly file %s.\n",
            FileName.c_str());

        return false;
    }

    if (!Disassembly.empty()) {
        if (fwrite(&Disassembly[0], Disassembly.size(), 1, f) != 1) {
            fclose(f);

            TextOut->WriteText(
                "Error: Failed to write to disassembly file %s.\n",
                FileName.c_str());

            return false;
        }
    }

    fclose(f);

    //
    // Now attempt to raise the script to the high level IR and print the IR out
    // as well.
    //
    // The script analyzer only operates on disk files, and the input file may
    // have come from the resource system, so we'll need to write it back out to
    // a temporary location first.
    //

    FileName = ResMan.GetResTempPath();
    FileName += "NWNScriptCompilerTempScript.ncs";

    f = fopen(FileName.c_str(), "wb");

    if (f == nullptr) {
        TextOut->WriteText(
            "Error: Unable to open script temporary file %s.\n",
            FileName.c_str());

        return false;
    }

    if (!InFileContents.empty()) {
        if (fwrite(&InFileContents[0], InFileContents.size(), 1, f) != 1) {
            fclose(f);

            TextOut->WriteText(
                "Error: Failed to write to script temporary file %s.\n",
                FileName.c_str());

            return false;
        }
    }

    ScriptTempFile = FileName;

    fclose(f);

    if (!DbgFileContents.empty()) {
        FileName = ResMan.GetResTempPath();
        FileName += "NWNScriptCompilerTempScript.ndb";

        f = fopen(FileName.c_str(), "wb");

        if (f == nullptr) {
            TextOut->WriteText(
                "Error: Unable to open symbols temporary file %s.\n",
                FileName.c_str());

            return false;
        }

        if (!DbgFileContents.empty()) {
            if (fwrite(&DbgFileContents[0], DbgFileContents.size(), 1, f) != 1) {
                fclose(f);

                TextOut->WriteText(
                    "Error: Failed to write to symbols temporary file %s.\n",
                    FileName.c_str());

                return false;
            }
        }

        fclose(f);

        SymbolsTempFile = FileName;
    }

    return true;
}

bool
ProcessInputFile(
    ResourceManager& ResMan,
    NscCompiler& Compiler,
    bool Compile,
    int CompilerVersion,
    bool Optimize,
    bool IgnoreIncludes,
    bool SuppressDebugSymbols,
    bool Quiet,
    bool VerifyCode,
    IDebugTextOut* TextOut,
    UINT32 CompilerFlags,
    const std::string& InFile,
    const std::string& OutBaseFile
)
/*++

Routine Description:

    This routine processes a single input file according to the desired compile
    or diassemble options.

Arguments:

    ResMan - Supplies the resource manager to use to service file load requests.

    NscCompiler - Supplies the compiler context that will be used to process the
                  request.

    Compile - Supplies a Boolean value indicating true if the input file is to
              be compiled, else false if it is to be disassembled.

    CompilerVersion - Supplies the BioWare-compatible compiler version number.

    Optimize - Supplies a Boolean value indicating true if the script should be
               optimized.

    IgnoreIncludes - Supplies a Boolean value indicating true if include-only
                     source files should be ignored.

    SuppressDebugSymbols - Supplies a Boolean value indicating true if debug
                           symbol generation should be suppressed.

    Quiet - Supplies a Boolean value that indicates true if non-critical
            messages should be silenced.

    VerifyCode - Supplies a Boolean value that indicates true if generated code
                 is to be verified with the analyzer/verifier if compilation was
                 successful.

    TextOut - Supplies the text out interface used to receive any diagnostics
              issued.

    CompilerFlags - Supplies compiler control flags.  Legal values are drawn
                    from the NscCompilerFlags enumeration.

    InFile - Supplies the path to the input file.

    OutBaseFile - Supplies the base name (potentially including path) of the
                  output file.  No extension is present.

Return Value:

    The routine returns a Boolean value indicating true on success, else false
    on failure.

    On catastrophic failure, an std::exception is raised.

Environment:

    User mode.

--*/
{
    NWN::ResRef32 FileResRef;
    NWN::ResType FileResType;
    std::vector<unsigned char> InFileContents;

    //
    // Pull in the input file first.
    //

    if (!LoadInputFile(
        ResMan,
        TextOut,
        InFile,
        FileResRef,
        FileResType,
        InFileContents)) {

        TextOut->WriteText("Error: Unable to read input file '%s'.\n", InFile.c_str());

        return false;
    }

    //
    // Now execute the main operation.
    //

    if (Compile) {
        return CompileSourceFile(
            Compiler,
            CompilerVersion,
            Optimize,
            IgnoreIncludes,
            SuppressDebugSymbols,
            Quiet,
            VerifyCode,
            TextOut,
            CompilerFlags,
            FileResRef,
            InFileContents,
            OutBaseFile);

    }
    else {
        std::vector<unsigned char> DbgFileContents;
        std::string DbgFileName;
        std::string::size_type Offs;

        DbgFileName = InFile;

        Offs = DbgFileName.find_last_of('.');

        if (Offs != std::string::npos) {
            NWN::ResRef32 DbgFileResRef;
            NWN::ResType DbgFileResType;

            DbgFileName.erase(Offs);
            DbgFileName += ".ndb";

            try {
                LoadInputFile(
                    ResMan,
                    TextOut,
                    DbgFileName,
                    DbgFileResRef,
                    DbgFileResType,
                    DbgFileContents);
            }
            catch (std::exception) {
            }
        }

        return DisassembleScriptFile(
            ResMan,
            Compiler,
            Quiet,
            TextOut,
            FileResRef,
            InFileContents,
            DbgFileContents,
            OutBaseFile);
    }
}

bool
ProcessWildcardInputFile(
    ResourceManager& ResMan,
    NscCompiler& Compiler,
    bool Compile,
    int CompilerVersion,
    bool Optimize,
    bool IgnoreIncludes,
    bool SuppressDebugSymbols,
    bool Quiet,
    bool VerifyCode,
    unsigned long Flags,
    IDebugTextOut* TextOut,
    UINT32 CompilerFlags,
    const std::string& InFile,
    const std::string& BatchOutDir
)
/*++

Routine Description:

    This routine processes a wildcard input file according to the desired
    compile or diassemble options.

Arguments:

    ResMan - Supplies the resource manager to use to service file load requests.

    NscCompiler - Supplies the compiler context that will be used to process the
                  request.

    Compile - Supplies a Boolean value indicating true if the input file is to
              be compiled, else false if it is to be disassembled.

    CompilerVersion - Supplies the BioWare-compatible compiler version number.

    Optimize - Supplies a Boolean value indicating true if the script should be
               optimized.

    IgnoreIncludes - Supplies a Boolean value indicating true if include-only
                     source files should be ignored.

    SuppressDebugSymbols - Supplies a Boolean value indicating true if debug
                           symbol generation should be suppressed.

    Quiet - Supplies a Boolean value that indicates true if non-critical
            messages should be silenced.

    VerifyCode - Supplies a Boolean value that indicates true if generated code
                 is to be verified with the analyzer/verifier if compilation was
                 successful.

    Flags - Supplies control flags that alter the behavior of the operation.
            Legal values are drawn from the NSCD_FLAGS enumeration.

            NscDFlag_StopOnError - Halt processing on first error.

    TextOut - Supplies the text out interface used to receive any diagnostics
              issued.

    CompilerFlags - Supplies compiler control flags.  Legal values are drawn
                    from the NscCompilerFlags enumeration.

    InFile - Supplies the path to the input file.  This may end in a wildcard.

    BatchOutDir - Supplies the batch compilation mode output directory.  This
                  may be empty (or else it must end in a path separator).

Return Value:

    The routine returns a Boolean value indicating true on success, else false
    on failure.

    On catastrophic failure, an std::exception is raised.

Environment:

    User mode.

--*/
{
    struct _finddata_t FindData;
    intptr_t FindHandle;
    std::string WildcardRoot;
    std::string MatchedFile;
    std::string OutFile;
    std::string::size_type Offs;
    bool Status;
    bool ThisStatus;
    unsigned long Errors;

    Errors = 0;

#if defined(_WINDOWS)
    char                   Drive[_MAX_DRIVE];
    char                   Dir[_MAX_DIR];
    char                   FileName[_MAX_FNAME];
    char                   Extension[_MAX_EXT];

    if (_splitpath_s(
        InFile.c_str(),
        Drive,
        Dir,
        FileName,
        Extension))
    {
        TextOut->WriteText(
            "Error: Malformed input wildcard path %s.\n", InFile.c_str());

        return false;
    }
#else
    char* Dir;
    char* FileName;
    std::string Extension;
    std::string Drive = "";

    char* dirc = strdup(InFile.c_str());
    char* filec = strdup(InFile.c_str());

    Dir = dirname(dirc);
    FileName = basename(filec);
    Extension = OsCompat::getFileExt(FileName);

#endif

    WildcardRoot = Drive;
    WildcardRoot += Dir;

    FindHandle = _findfirst(InFile.c_str(), &FindData);

    if (FindHandle == -1) {
        TextOut->WriteText(
            "Error: No matching files for input wildcard path %s.\n",
            InFile.c_str());

        return false;
    }

    Status = true;

    //
    // Operate over all files matching the wildcard, performing the requested
    // compile or disassemble operation.
    //

    do {
        if (FindData.attrib & _A_SUBDIR)
            continue;

        MatchedFile = WildcardRoot;

#if defined(_WINDOWS)
        if (MatchedFile.length() > 0) {
            if (MatchedFile.back() != '\\')
                MatchedFile.push_back('\\');
        }
#else
        if (MatchedFile.back() != '/')
            MatchedFile.push_back('/');
#endif

        MatchedFile += FindData.name;

        if (BatchOutDir.empty()) {
            OutFile = MatchedFile;
        }
        else {
            OutFile = BatchOutDir;
            OutFile += FindData.name;
        }

        Offs = OutFile.find_last_of('.');

        if (Offs != std::string::npos)
            OutFile.erase(Offs);

        ThisStatus = ProcessInputFile(
            ResMan,
            Compiler,
            Compile,
            CompilerVersion,
            Optimize,
            IgnoreIncludes,
            SuppressDebugSymbols,
            Quiet,
            VerifyCode,
            &g_TextOut,
            CompilerFlags,
            MatchedFile,
            OutFile);

        if (!ThisStatus) {
            TextOut->WriteText(
                "Error: Failed to process file %s.\n",
                MatchedFile.c_str());

            Status = false;

            Errors += 1;

            if (Flags & NscDFlag_StopOnError) {
                TextOut->WriteText("Stopping processing on first error.\n");
                break;
            }
        }
    } while (!_findnext(FindHandle, &FindData));

    _findclose(FindHandle);

    if (Errors)
        TextOut->WriteText("%d error(s); see above for context.\n", Errors);

    return Status;
}

bool
LoadResponseFile(
    int argc,
    char** argv,
    const char* ResponseFileName,
    StringVec& Args,
    StringArgVec& ArgVector
)
/*++

Routine Description:

    This routine loads command line arguments from a response file.  Each line
    represents an argument.  The contents are read into a vector for later
    processing.

Arguments:

    argc - Supplies the original command line argument count.

    argv - Supplies the original command line argument vector.

    ResponseFileName - Supplies the file name of the response file.

    Args - Received the lines in the response file.

    ArgVector - Receives an array of pointers to the each line in Args.

Return Value:

    The routine returns a Boolean value indicating true if the response file was
    loaded, else false if an error occurred.

Environment:

    User mode.

--*/
{
    FILE* f;

    f = nullptr;

    try {
        char Line[1025];

        f = fopen(ResponseFileName, "rt");

        if (f == nullptr)
            throw std::runtime_error("Failed to open response file.");

        //
        // Tokenize the file into lines and then build a pointer array that is
        // consistent with the standard 'main()' contract.  The first argument
        // is copied from the main argument array, if it exists (i.e. the
        // program name).
        //

        if (argc > 0)
            Args.push_back(argv[0]);

        while (fgets(Line, sizeof(Line) - 1, f)) {
            std::ignore = strtok(Line, "\r\n");

            if (!Line[0])
                continue;

            Args.push_back(Line);
        }

        //
        // N.B.  Beyond this point no modifications may be made to Args as we
        //       are creating pointers into the data storage of each member for
        //       the remainder of the function.
        //

        ArgVector.reserve(Args.size());

        for (std::vector<std::string>::const_iterator it = Args.begin();
            it != Args.end();
            ++it) {
            ArgVector.push_back(it->c_str());
        }

        return true;
    }
    catch (std::exception& e) {
        if (f != nullptr) {
            fclose(f);
            f = nullptr;
        }

        g_TextOut.WriteText(
            "Error: Exception parsing response file '%s': '%s'.\n",
            ResponseFileName,
            e.what());

        return false;
    }
}


int
main(
    int argc,
    char** argv
)
/*++

Routine Description:

    This routine initializes and executes the script compiler.

Arguments:

    argc - Supplies the count of command line arguments.

    argv - Supplies the command line argument array.

Return Value:

    On success, zero is returned; otherwise, a non-zero value is returned.
    On catastrophic failure, an std::exception is raised.

Environment:

    User mode.

--*/
{
    std::vector<std::string> SearchPaths;
    std::vector<std::string> InFiles;
    std::string OutFile;
    std::string ModuleName;
    std::string InstallDir;
    std::string HomeDir;
    std::string ErrorPrefix;
    std::string BatchOutDir;
    std::string CustomModPath;
    StringVec ResponseFileText;
    StringArgVec ResponseFileArgs;
    bool Compile = true;
    bool Optimize = false;
    bool EnableExtensions = false;
    bool NoDebug = true;
    bool Quiet = false;
    int CompilerVersion = 174;
    bool Error = false;
    bool LoadResources = false;
    bool Erf16 = true;
    bool ResponseFile = false;
    int ReturnCode = 0;
    bool VerifyCode = false;
    bool Usage = false;
    unsigned long Errors = 0;
    unsigned long Flags = NscDFlag_StopOnError;
    UINT32 CompilerFlags = 0;
    //    bool logInfo = false;
    //    bool logWarn = false;
    //    bool logDebug = false;
    //    bool logTrace = false;

    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.set(el::Level::Info,
        el::ConfigurationType::Enabled, "false");
    defaultConf.set(el::Level::Warning,
        el::ConfigurationType::Enabled, "false");
    //    defaultConf.set(el::Level::Verbose,
    //                    el::ConfigurationType::Enabled, "false");
    defaultConf.set(el::Level::Debug,
        el::ConfigurationType::Enabled, "false");
    defaultConf.set(el::Level::Trace,
        el::ConfigurationType::Enabled, "false");
    defaultConf.set(el::Level::Global, el::ConfigurationType::ToFile, "false");
    el::Loggers::addFlag(el::LoggingFlag::LogDetailedCrashReason);

#if defined(_WINDOWS)
    ULONGLONG StartTime = GetTickCount64();
#else
    std::chrono::high_resolution_clock::time_point StartTime = std::chrono::high_resolution_clock::now();
#endif

    SearchPaths.emplace_back(".");

    do {
        //
        // Parse arguments out.
        //

        for (int i = 1; i < argc && !Error; i += 1) {
            //
            // If it's a switch, consume it.  Otherwise it is an input file.
            //

            if (argv[i][0] == '-') {
                const char* Switches;
                char Switch;

                Switches = &argv[i][1];

                while ((*Switches != '\0') && (!Error)) {
                    Switch = *Switches++;

                    switch ((wint_t)(unsigned)Switch) {

                    case 'a':
                        VerifyCode = true;
                        break;

                    case 'b': {
                        if (i + 1 >= argc) {
                            g_TextOut.WriteText("Error: Malformed arguments.\n");
                            Error = true;
                            break;
                        }

                        BatchOutDir = argv[i + 1];

                        if (BatchOutDir.empty())
                            BatchOutDir = ".";

#if defined(_WINDOWS)
                        if (BatchOutDir.back() != '\\')
                            BatchOutDir.push_back('\\');
#else
                        if (BatchOutDir.back() != '/')
                            BatchOutDir.push_back('/');
#endif


                        i += 1;
                    }
                            break;

                    case 'd':
                        Compile = false;
                        break;

                    case 'e':
                        EnableExtensions = true;
                        break;

                    case 'g':
                        NoDebug = false;
                        break;

                    case 'h': {
                        if (i + 1 >= argc) {
                            g_TextOut.WriteText("Error: Malformed arguments.\n");
                            Error = true;
                            break;
                        }

                        HomeDir = argv[i + 1];

#if defined(_WINDOWS)
                        if (HomeDir.back() != '\\')
                            HomeDir.push_back('\\');
#else
                        if (HomeDir.back() != '/')
                            HomeDir.push_back('/');
#endif

                        i += 1;
                    }
                            break;

                    case 'i': {
                        char* Token = nullptr;
                        char* NextToken = nullptr;
                        std::string Ansi;

                        if (i + 1 >= argc) {
                            g_TextOut.WriteText("Error: Malformed arguments.\n");
                            Error = true;
                            break;
                        }

                        for (Token = strtok_r(argv[i + 1], ";", &NextToken);
                            Token != nullptr;
                            Token = strtok_r(nullptr, ";", &NextToken)) {
                            SearchPaths.emplace_back(Token);
                        }

                        i += 1;
                    }
                            break;

                    case 'D':
                        defaultConf.set(el::Level::Debug,
                            el::ConfigurationType::Enabled, "true");
                        break;

                    case 'I':
                        defaultConf.set(el::Level::Info,
                            el::ConfigurationType::Enabled, "true");
                        break;

                    case 'W':
                        defaultConf.set(el::Level::Warning,
                            el::ConfigurationType::Enabled, "true");
                        break;

                    case 'T':
                        defaultConf.set(el::Level::Trace,
                            el::ConfigurationType::Enabled, "true");
                        break;

                    case 'j':
                        CompilerFlags |= NscCompilerFlag_ShowIncludes;
                        break;

                    case 'k':
                        CompilerFlags |= NscCompilerFlag_ShowPreprocessed;
                        break;

                    case 'l':
                        LoadResources = true;
                        break;

                    case 'n': {
                        LoadResources = true;
                        if (i + 1 >= argc) {
                            g_TextOut.WriteText("Error: Malformed arguments.\n");
                            Error = true;
                            break;
                        }

                        InstallDir = argv[i + 1];

#if defined(_WINDOWS)
                        if (InstallDir.back() != '\\')
                            InstallDir.push_back('\\');
#else
                        if (InstallDir.back() != '/')
                            InstallDir.push_back('/');
#endif

                        i += 1;
                    }
                            break;

                    case 'm': {
                        CompilerVersion = 0;

                        const char* version = argv[i + 1];

                        while (*version != '\0') {
                            char Digit = *version++;

                            if (isdigit((wint_t)(unsigned)Digit)) {
                                CompilerVersion = CompilerVersion * 10 + (Digit - '0');
                            }
                            else if (Digit == '.') {
                                //
                                // Permitted, but ignored.
                                //
                            }
                            else {
                                g_TextOut.WriteText(
                                    "Error: Invalid digit in version number.\n");
                                Error = true;
                                break;
                            }
                        }
                        i += 1;
                    }
                            break;

                    case 'o':
                        Optimize = true;
                        break;

                    case 'p':
                        CompilerFlags |= NscCompilerFlag_DumpPCode;
                        break;

                    case 'q':
                        Quiet = true;
                        break;

                    case 'r': {
                        if (i + 1 >= argc) {
                            g_TextOut.WriteText("Error: Malformed arguments.\n");
                            Error = true;
                            break;
                        }

                        OutFile = argv[i + 1];

                        i += 1;
                    }
                            break;

                    case 's':
                        CompilerFlags |= NscCompilerFlag_StrictModeEnabled;
                        break;

                    case 'v':
                        Usage = true;
                        break;

                    case 'w':
                        CompilerFlags |= NscCompilerFlag_SuppressWarnings;
                        break;

                    case 'x': {
                        if (i + 1 >= argc) {
                            g_TextOut.WriteText("Error: Malformed arguments.\n");
                            Error = true;
                            break;
                        }

                        ErrorPrefix = argv[i + 1];

                        i += 1;
                    }
                            break;

                    case 'y':
                        Flags &= ~(NscDFlag_StopOnError);
                        break;

                    case 'M':
                        CompilerFlags |= NscCompilerFlag_GenerateMakeDeps;
                        break;

                    case 'Q':
                        CompilerFlags |= NscCompilerFlag_DisableDoubleQuote;
                        break;

                    default: {
                        g_TextOut.WriteText("Error: Unrecognized option \"%c\".\n", Switch);
                        Error = true;
                    }
                           break;

                    }
                }
            }
            else if (argv[i][0] == '@') {
                if (ResponseFile) {
                    g_TextOut.WriteText("Error: Nested response files are unsupported.\n");
                    Error = true;
                    break;
                }

                if (!LoadResponseFile(
                    argc,
                    argv,
                    &argv[i][1],
                    ResponseFileText,
                    ResponseFileArgs)) {
                    Error = true;
                    break;
                }

                ResponseFile = true;
            }
            else {
                std::string Ansi;

                Ansi = argv[i];

                InFiles.push_back(Ansi);

            }
        }

        if (ResponseFile) {
            //
            // If we have no response file data, then stop parsing.  The first
            // element is a duplicate of argv[ 0 ].
            //

            if (ResponseFileArgs.size() < 2)
                break;

            //
            // If we just finished parsing the response file arguments, then we
            // are done.
            //

            if (argv[0] == ResponseFileArgs[0])
                break;

            argc = (int)ResponseFileArgs.size();

            if (argc < 1)
                break;

            argv = (char**)&ResponseFileArgs[0];
        }
        else {
            break;
        }
    } while (!Error);


    if ((Usage) || (Error) || (InFiles.empty())) {
        g_TextOut.WriteText(
            "\nUsage: version %s - built %s %s\n\n"
            "nwnsc [-degjklorsqvwyM] [-b batchoutdir] [-h homedir] [-i pathspec] [-n installdir]\n"
            "      [-m mode] [-x errprefix] [-r outfile] infile [infile...]\n\n"
            "  -b batchoutdir - Supplies the location where batch mode places output files\n"
            "  -h homedir     - Per-user NWN home directory (i.e. Documents\\Neverwinter Nights)\n"
            "  -i pathspec    - Semicolon separated list of folders to search for additional includes\n"
            "  -n installdir  - Neverwinter Nights install folder. Use to load base game includes\n"
            "  -m mode        - Compiler mode 1.69 or 1.74 - (default 1.74) \n"
            "  -x errprefix   - Prefix string to prepend to compiler errors (default \"Error\")\n\n"
            "  -d - Disassemble the script (overrides default compile\n"
            "  -e - Enable non-BioWare extensions\n"
            "  -g - Enable generation of .ndb debug symbols file\n"
            "  -j - Show where include file are being sourced from\n"
            "  -k - Show preprocessed source text to console output\n"
            "  -l - Load base game resources - not required with -n\n"
            "  -o - Optimize the compiled script\n"
            "  -p - Dump internal PCode for compiled script contributions\n"
            "  -q - Silence most messages\n"
            "  -r - Filename for output file\n"
            "  -s - Enable Strict mode. This enables stock compiler compatibility that allows\n"
            "       some potentially unsafe conditions (default: off)\n"
            "  -v - Version and detailed usage message\n"
            "  -w - Suppress compile warnings (default: false)\n"
            "  -y - Continue processing input files even on error\n"
            "  -M - Create makefile dependency (.d) files\n"
            "  -Q - Disable the parsing of \\\" and \\\\ (added in NWN EE) \n\n"
            "  The Compiler requires the nwscript.nss from the game resources. The following order\n"
            "      will be followed to find the file. The search stops on the first match.\n"
            "    1. -i pathspec  The pathspec will be searched as the game scipts may\n"
            "            be unpacked into a flat folder structure\n"
            "    2. -n installdir will be searched.  The search starts with <installdir>/ovr\n"
            "            and continues with <installdir>/data/*.bif files\n"
            "    3. -l If the -l flag is passed The following sources are searched:\n"
            "            Check the environment variable NWN_ROOT\n"
            "            NWN EE Digital Deluxe Beta (Head Start)\n"
            "            NWN EE Beta (Head Start)\n"
            "            NWN EE Digital Deluxe\n"
            "            NWN EE \n"
            "            NWN Steam\n\n",
            gGIT_VERSION_SHORT.c_str(),
            __DATE__,
            __TIME__

        );
        if (Usage) {
            g_TextOut.WriteText(
                "Optional debug flags\n"
                "  -D - Set Debug Log level\n"
                "  -I - Set Info Log level\n"
                "  -W - Set Warning Log level\n"
                "  -T - Set Trace Log level\n"
            );

            g_TextOut.WriteText("\n"
                "  Portions Copyright (C) 2008-2015 Skywing\n"
                "  Portions copyright (C) 2002-2003, Edward T. Smith\n"
                "  Portions copyright (C) 2003, The Open Knights Consortium\n"
                "  Adapted for Neverwinter Nights Enhanced Edition and cross platform use by: Glorwinger and Jakkn\n");
        }

        return -1;
    }

    el::Loggers::reconfigureLogger("default", defaultConf);

    //
    // Create the resource manager context and load the module, if we are to
    // load one.
    //

    try {
        g_ResMan = new ResourceManager(&g_TextOut);
    }
    catch (std::runtime_error& e) {
        g_TextOut.WriteText(
            "Failed to initialize resource manager: '%s'\n",
            e.what());

        if (g_Log != nullptr) {
            fclose(g_Log);
            g_Log = nullptr;
        }

        return 0;
    }

    if (LoadResources) {
        //
        // If we're to load game resources, then do so now.
        //

        if (!Quiet)
        {
            g_TextOut.WriteText("Loading base game resources...\n");
        }

        if (InstallDir.empty()) {
            InstallDir = GetNwnInstallPath(CompilerVersion, Quiet);
        }

        if (HomeDir.empty())
            HomeDir = GetNwnHomePath(CompilerVersion, Quiet);

        LoadScriptResources(
            *g_ResMan,
            HomeDir,
            InstallDir,
            Erf16,
            CompilerVersion);

        if (CompilerVersion >= 174) {
            std::string Override = InstallDir + "ovr";

#if defined(_WINDOWS)
            if (Override.back() != '\\')
                Override.push_back('\\');
#else
            if (Override.back() != '/')
                Override.push_back('/');
#endif
            SearchPaths.push_back(Override);
        }
    }

    //
    // Now create the script compiler context.
    //

    NscCompiler Compiler(*g_ResMan, EnableExtensions);

    if (!SearchPaths.empty())
        Compiler.NscSetIncludePaths(SearchPaths);

    if (!ErrorPrefix.empty())
        Compiler.NscSetCompilerErrorPrefix(ErrorPrefix.c_str());

    Compiler.NscSetResourceCacheEnabled(true);

    //
    // Process each of the input files in turn.
    //

    for (std::vector<std::string>::const_iterator it = InFiles.begin();
        it != InFiles.end();
        ++it) {
        std::string ThisOutFile;
        std::string::size_type Offs;
        bool Status;

        //
        // Load the source text and compile the program.
        //

        if (it->find_first_of("*?") != std::string::npos) {
            //
            // We've a wildcard, process it appropriately.
            //

            Status = ProcessWildcardInputFile(
                *g_ResMan,
                Compiler,
                Compile,
                CompilerVersion,
                Optimize,
                true,
                NoDebug,
                Quiet,
                VerifyCode,
                Flags,
                &g_TextOut,
                CompilerFlags,
                *it,
                BatchOutDir);
        }
        else {
            if (BatchOutDir.empty()) {
                ThisOutFile = OutFile;

                if (ThisOutFile.empty())
                    ThisOutFile = *it;

                Offs = ThisOutFile.find_last_of('.');

                if (Offs != std::string::npos)
                    ThisOutFile.erase(Offs);
            }
            else {

#if defined(_WINDOWS)
                char FileName[_MAX_FNAME];

                if (_splitpath_s(
                    it->c_str(),
                    nullptr,
                    0,
                    nullptr,
                    0,
                    FileName,
                    _MAX_FNAME,
                    nullptr,
                    0))
                {
                    g_TextOut.WriteText(
                        "Error: Invalid path: \"%s\".\n",
                        it->c_str());

                    ReturnCode = -1;
                    continue;
                }
#else
                char filec[_MAX_FNAME];
                strncpy(filec, it->c_str(), _MAX_FNAME);
                char* FileName = OsCompat::filename(filec);
#endif

                ThisOutFile = BatchOutDir;
                ThisOutFile += FileName;
            }

            //
            // We've a regular (single) file name, process it.
            //

            Status = ProcessInputFile(
                *g_ResMan,
                Compiler,
                Compile,
                CompilerVersion,
                Optimize,
                true,
                NoDebug,
                Quiet,
                VerifyCode,
                &g_TextOut,
                CompilerFlags,
                *it,
                ThisOutFile);
        }

        if (!Status) {
            ReturnCode = -1;

            Errors += 1;

            if (Flags & NscDFlag_StopOnError) {
                g_TextOut.WriteText("Processing aborted.\n");
                break;
            }
        }
    }

#if defined(_WINDOWS)
    if (!Quiet)
    {
        double durationFloat = (float)(GetTickCount64() - StartTime) / (float)1000;
        g_TextOut.WriteText(
            "Total Execution time = %.4f seconds\n",
            durationFloat);
    }
#else
    std::chrono::high_resolution_clock::time_point EndTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(EndTime - StartTime).count();
    double durationFloat = (float)duration / (float)1000;
    if (!Quiet)
    {
        g_TextOut.WriteText("Total Execution time = %.4f seconds  \n", durationFloat);
    }
#endif

    if (Errors > 1)
        g_TextOut.WriteText("%lu error(s) processing input files.\n", Errors);

    if (g_Log != nullptr) {
        fclose(g_Log);
        g_Log = nullptr;
    }

    //
    // Now tear down the system.
    //

    delete g_ResMan;
    g_ResMan = nullptr;

    return ReturnCode;
}



