/** @file NWScriptCompiler.cpp
 * Invokes various functions from NscLib compiler/interpreter library.
 *
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.


#include <fstream>

#include "NWScriptCompiler.h"

using namespace NWScriptPlugin;


std::vector<std::pair<std::string, std::string>> NwnVersions = {
    { "00840", "NWN EE Digital Deluxe Beta (Head Start)" },
    { "00829", "NWN EE Beta (Head Start)" },
    { "00839", "NWN EE Digital Deluxe" },
    { "00785", "NWN EE" }
};


