/** @file NWScriptCompiler.cpp
 * Invokes various functions from NscLib compiler/interpreter library.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include "pch.h"
//#include <fstream>
//#include "jpcre2.h"

#include "NWScriptCompiler.h"

using namespace NWScriptPlugin;
typedef NWScriptLogger::LogType LogType;

#define SCRIPTERRORPREFIX "Error"
#define FORMATDISASMREGEX ""

typedef jpcre2::select<char> pcre2;
static pcre2::Regex assemblyLine(R"(.+)", PCRE2_MULTILINE, jpcre2::JIT_COMPILE);

bool NWScriptCompiler::initialize(Settings* settings) {

    // Critical path, initialize resources
    try
    {
        _resourceManager = std::make_unique<ResourceManager>(&_logger);
    }
    catch (std::runtime_error& e)
    {
        _logger.log("Failed to initialize the resources manager:", LogType::Critical, "NSC2001", true);
        _logger.log(e.what(), LogType::Critical, true);
        return false;
    }

    _settings = settings;
    NWNHome = getNwnHomePath(_settings->compileVersion);

    return true;
}

bool NWScriptCompiler::LoadScriptResources()
{
    ResourceManager::ModuleLoadParams LoadParams;
    ResourceManager::StringVec KeyFiles;

    ZeroMemory(&LoadParams, sizeof(LoadParams));

    LoadParams.SearchOrder =  ResourceManager::ModSearch_PrefDirectory;
    LoadParams.ResManFlags =  ResourceManager::ResManFlagNoGranny2;
    LoadParams.ResManFlags |= ResourceManager::ResManFlagErf16;

    if (_settings->compileVersion == 174) 
    {
#ifdef _WINDOWS
        KeyFiles.push_back("data\\nwn_base");
#else
        KeyFiles.emplace_back("data/nwn_base");
#endif // _WINDOWS
    }
    else 
    {
        KeyFiles.emplace_back("xp3");
        KeyFiles.emplace_back("xp2patch");
        KeyFiles.emplace_back("xp2");
        KeyFiles.emplace_back("xp1");
        KeyFiles.emplace_back("chitin");
    }

    LoadParams.KeyFiles = &KeyFiles;
    LoadParams.ResManFlags |= ResourceManager::ResManFlagBaseResourcesOnly;

    // Legacy code is using ASCII string names. We convert here. Also, many exceptions thrown inside those classes to deal with.
    std::string InstallDir = _settings->getChosenInstallDir();
    try {
        _resourceManager->LoadScriptResources(wstr2str(NWNHome), InstallDir, &LoadParams);
    }
    catch(...) { 
        //_resourceManager is writting to the log messages here, so we just return false.
        return false;
    }

    return true;
}

bool NWScriptCompiler::processFile(filesystem::path& sourcePath, filesystem::path& destDir, bool fromMemory, char* fileContents)
{
    _logger.log("Initializing process...", LogType::ConsoleMessage);

    // First check... is the compiler initialized?
    if (!isInitialized())
    {
        _logger.log("NWScriptCompiler is not initialized!", LogType::Critical, "NSC2000");
        return false;
    }

    // Start building up search paths. 
    if (!includePathsBuilt)
    {
        _includePaths.push_back(wstr2str(sourcePath.parent_path()));

        if (!_settings->ignoreInstallPaths)
        {
            if (!LoadScriptResources())
            {
                _logger.log("Could not load script resources on installation path: " + _settings->getChosenInstallDir(), LogType::Warning);
            }

            if (_settings->compileVersion == 174)
            {
                std::string overrideDir = _settings->getChosenInstallDir() + "ovr\\";
                _includePaths.push_back(overrideDir);
            }
        }

        for (generic_string s : _settings->getIncludeDirsV())
        {
            _includePaths.push_back(wstr2str(properDirName(s)));
        }
    }

    if (_settings->compileMode == 0)
        _logger.log("Compiling script:" + wstr2str(sourcePath), LogType::Info);
    else
        _logger.log("Disassembling binary:" + wstr2str(sourcePath), LogType::Info);

    // Acquire information about NWN Resource Type of the file.
    NWN::ResType fileResType;
    NWN::ResRef32 fileResRef;

    fileResType = _resourceManager->ExtToResType(wstr2str(sourcePath).c_str());
    fileResRef = _resourceManager->ResRef32FromStr(wstr2str(sourcePath).c_str());

    std::string inFileContents;
    if (fromMemory)
        inFileContents = fileContents;
    else
    {
        if (!fileToBuffer(sourcePath.c_str(), inFileContents))
        {
            _logger.log("Could not load the specified file: " + wstr2str(sourcePath), LogType::Error, "NSC2002");
            return false;
        }
    }

    // Create our compiler/disassembler
    if (!_compilerCreated)
    {
        _compiler = std::make_unique<NscCompiler>(*_resourceManager, _settings->useNonBiowareExtenstions);
        _compiler->NscSetIncludePaths(_includePaths);
        _compiler->NscSetCompilerErrorPrefix(SCRIPTERRORPREFIX);
        _compiler->NscSetResourceCacheEnabled(true);
        _compilerCreated = true;
    }

    // Execute the process
    if (_settings->compileMode == 0)
    {
        _logger.log("Compiling script: " + sourcePath.string(), LogType::ConsoleMessage);
        return compileScript(sourcePath, destDir, fromMemory, inFileContents, fileResType, fileResRef);
    }
    else
    {
        _logger.log("Disassembling binary: " + sourcePath.string(), LogType::ConsoleMessage);
        return disassembleBinary(sourcePath, destDir, fromMemory, inFileContents, fileResType, fileResRef);
    }
}


bool NWScriptCompiler::compileScript(filesystem::path& sourcePath, filesystem::path& destDir, bool fromMemory, std::string& fileContents,
    const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef)
{
    // Sanity check
    if (!_compilerCreated)
        return false;

    // We always ignore include files. 
    bool bIgnoreIncludes = true;

    // Main compilation step
    std::string dataRef;
    swutil::ByteVec generatedCode;
    swutil::ByteVec debugSymbols;
    std::set<std::string> fileDependencies;

    NscResult result = _compiler->NscCompileScript(fileResRef, fileContents.c_str(), fileContents.size(), _settings->compileVersion,
        _settings->optimizeScript, bIgnoreIncludes, &_logger, _settings->compilerFlags, generatedCode, debugSymbols, fileDependencies);

    switch (result)
    {
    case NscResult_Failure:
    {
        _logger.log("Compilation aborted with errors.", LogType::ConsoleMessage);
        return false;
    }

    case NscResult_Include:
    {
        _logger.log(sourcePath.filename().string() + " is an include file, ignored.", LogType::ConsoleMessage);
        return true;
    }

    case NscResult_Success:
        break;

    default:
        _logger.log("Unknown status code", LogType::ConsoleMessage);
        return false;
    }

    // Now save code data
    generic_string outputPath = str2wstr(destDir.string() + sourcePath.stem().string() + ".ncs");
    dataRef.assign(reinterpret_cast<char*>(&generatedCode[0]), generatedCode.size());
    if (!bufferToFile(outputPath, dataRef))
    {
        _logger.log(TEXT("Error: could not write compiled output file: ") + outputPath, LogType::ConsoleMessage);
        return false;
    }

    // Save debug symbols if apply
    if (_settings->generateSymbols)
    {
        outputPath = str2wstr(destDir.string() + sourcePath.stem().string() + ".ndb");
        dataRef.assign(reinterpret_cast<char*>(&debugSymbols[0]), debugSymbols.size());
        if (!bufferToFile(outputPath, dataRef))
        {
            _logger.log(TEXT("Error: could not write generated symbols output file: ") + outputPath, LogType::ConsoleMessage);
            return false;
        }
    }

    // And file dependencies if apply
    if (_settings->compilerFlags & NscCompilerFlag_GenerateMakeDeps)
    {
        std::stringstream sdependencies;
        sdependencies << sourcePath.stem().string() << ".ncs " << sourcePath.stem().string() << ".nss " << R"(\r\n)";

        for (auto& dependency : fileDependencies)
            sdependencies << "\\\r\n    " << dependency;

        for (auto& dependency : fileDependencies)
            sdependencies << R"(\r\n)" << dependency << ":" << R"(\r\n)";

        outputPath = str2wstr(destDir.string() + sourcePath.stem().string() + "nss.dependencies");       
        if (!bufferToFile(outputPath, sdependencies.str()))
        {
            _logger.log(TEXT("Error: could not write dependency file: ") + outputPath, LogType::ConsoleMessage);
            return false;
        }
    }

    return true;
}


bool NWScriptCompiler::disassembleBinary(filesystem::path& sourcePath, filesystem::path& destDir, bool fromMemory, std::string& fileContents,
    const NWN::ResType& fileResType, const NWN::ResRef32& fileResRef)
{
    // Sanity check
    if (!_compilerCreated)
        return false;

    std::string generatedCode;

    // Main disassemble step.
    _compiler->NscDisassembleScript(fileContents.c_str(), fileContents.size(), generatedCode);

    // This is the way the library returns errors to us on that routine... :D
    if (generatedCode == "DISASSEMBLY ERROR: COMPILER INITIALIZATION FAILED!")
    {
        _logger.log("Error disassembling file. Compiler Initialization failed!", LogType::Error);
        return false;
    }

    // Save file, but first, we remove extra carriage returns the library is generating...
    generic_string outputPath = str2wstr(destDir.string() + sourcePath.stem().string() + ".ncs.pcode");
    
    std::stringstream formatedCode;
    pcre2::VecNum matches;
    pcre2::RegexMatch fileMatcher(&assemblyLine);
    size_t lineCount = fileMatcher.setSubject(generatedCode).setModifier("gm").setNumberedSubstringVector(&matches).match();

    for (size_t i = 0; i < lineCount; i++)
        formatedCode << matches[i][0] << "\r\n";

    if (!bufferToFile(outputPath, formatedCode.str()))
    {
        _logger.log(TEXT("Error: could not write disassembled output file: ") + outputPath, LogType::ConsoleMessage);
        return false;
    }
}