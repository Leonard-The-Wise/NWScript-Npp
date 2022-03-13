/** @file ProjectVersion.h
 * Records build versions.
 * Pre-included into Read-only symbol directives on Resources page for resource file ProjectVersion.rc 
 * 
 * Major Version: Must be set manually
 * Minor Version: Also set manually
 * Patch: Auto-updated on every Git Commit. Recommend to Zero it on a new Minor or Major version change.
 * Build: Auto-updated by "IncrementBuild.ps1" script when pre-build step runs. Manual changes not needed.
 * 
 * WARNING: If you edit the ProjectVersion.rc file in Visual Studio, you'll lose the macros inside
 * that will auto-update VS_VERSION_INFO with values from here. 
 * 
 * Again, don't edit ProjectVersion.rc inside Visual Studio EVER... I warned you.
 * 
 **/
 // Copyright (C) 2022 - Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#pragma once

#define stringify_(a) #a
#define stringify(a) stringify_(a)

#define VERSION_MAJOR 0
#define VERSION_MINOR 6
#define VERSION_PATCH 3
#define VERSION_BUILD 951

#define VERSION_STRING stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." \
	stringify(VERSION_PATCH) "." stringify(VERSION_BUILD)
#define VERSION_STRINGT TEXT(VERSION_STRING) 

#define VERSION_STRING_BUILD stringify(VERSION_MAJOR) "." stringify(VERSION_MINOR) "." \
	stringify(VERSION_PATCH) " build " stringify(VERSION_BUILD)
#define VERSION_STRING_BUILDT TEXT(VERSION_STRING_BUILD)


