/** @file NWScriptCompiler.cpp
 * Invokes various functions from NscLib compiler/interpreter library.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include "pch.h"
//#include <fstream>

#include "NWScriptCompiler.h"

using namespace NWScriptPlugin;
typedef NWScriptLogger::LogType LogType;

std::vector<std::pair<std::string, std::string>> NwnVersions = {
    { "00840", "NWN EE Digital Deluxe Beta (Head Start)" },
    { "00829", "NWN EE Beta (Head Start)" },
    { "00839", "NWN EE Digital Deluxe" },
    { "00785", "NWN EE" }
};


bool NWScriptCompiler::Initialize(Settings* settings) {

    // Critical path, initialize resources
    try
    {
        _resourceManager = std::make_unique<ResourceManager>(&logger);
    }
    catch (std::runtime_error& e)
    {
        logger.log(TEXT("Failed to initialize the resources manager:"), LogType::Critical, true);
        logger.log(str2wstr(e.what()), LogType::Critical, true);
        return false;
    }

    _settings = settings;
    NWNHome = GetNwnHomePath(_settings->compileVersion);

    return true;
}

bool NWScriptCompiler::LoadScriptResources()
{
    ResourceManager::ModuleLoadParams LoadParams;
    ResourceManager::StringVec KeyFiles;

    ZeroMemory(&LoadParams, sizeof(LoadParams));

    LoadParams.SearchOrder = ResourceManager::ModSearch_PrefDirectory;
    LoadParams.ResManFlags = ResourceManager::ResManFlagNoGranny2;
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
    std::string InstallDir = _settings->neverwinterInstallChoice == 0 ? wstr2str(_settings->neverwinterOneInstallDir) : wstr2str(_settings->neverwinterTwoInstallDir);
    try {
        _resourceManager->LoadScriptResources(wstr2str(NWNHome), InstallDir, &LoadParams);
    }
    catch(...) { 
        //logger.log
        return false;
    }

    ResourcesLoaded = true;
    return true;
}

bool NWScriptCompiler::CompileScript(filesystem::path& scriptPath)
{
    logger.log(TEXT("Initializing compilation process."), LogType::ConsoleMessage);

    // First check... is the compiler initialized?
    if (!isInitialized())
    {
        logger.log(TEXT("Compiler is not initialized!"), LogType::Critical, TEXT("NSC2000"));
        return false;
    }

    // Start building up search paths. (required at least the ".")
    std::vector<std::string> includePaths;
    includePaths.emplace_back(".");

    // Check if it's up to us to load game resources
    if (!ResourcesLoaded)
    {
        if (!_settings->ignoreInstallPaths)
        {
            if (!LoadScriptResources())
            {
                //logger.log()
            }


        }
    }


    for (generic_string s : _settings->getIncludeDirsV())
    {

    }

    NscCompiler Compiler(*_resourceManager, _settings->useNonBiowareExtenstions);

    return true;
}