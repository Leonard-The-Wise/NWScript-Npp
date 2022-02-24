/** @file DllMain.cxx
 * The main DLL entry point and basic plugin setup. 
 * All message handles are processed in PluginMain.cxx
 * 
 **/
 // Copyright 2022 by Leonardo Silva 
 // The License.txt file describes the conditions under which this software may be distributed.

#include "PluginInterface.h"
#include "PluginMain.h"

using namespace NWScriptPlugin;

BOOL APIENTRY DllMain(HANDLE hModule, DWORD  reasonForCall, LPVOID /*lpReserved*/)
{
    switch (reasonForCall)
    {
      case DLL_PROCESS_ATTACH:
		  Plugin::PluginInit(hModule);
		  break;

      case DLL_PROCESS_DETACH:
		  Plugin::PluginCleanUp();
		  break;

      case DLL_THREAD_ATTACH:
		  break;

      case DLL_THREAD_DETACH:
		  break;
    }

    return TRUE;
}


extern "C" DLLAPI void setInfo(NppData notpadPlusData)
{
	Plugin::Instance().SetNotepadData(notpadPlusData);
}

extern "C" DLLAPI const TCHAR* getName()
{
	return Plugin::Instance().GetName();
}

extern "C" DLLAPI FuncItem* getFuncsArray(int *nbF)
{
	*nbF = Plugin::Instance().GetFunctionCount();
	return Plugin::Instance().GetFunctions();
}

extern "C" DLLAPI void beNotified(SCNotification *notifyCode)
{
	Plugin::Instance().ProcessMessagesSci(notifyCode);
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" DLLAPI LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{
	return Plugin::Instance().ProcessMessagesNpp(Message, wParam, lParam);
}

#ifdef UNICODE
extern "C" DLLAPI BOOL isUnicode()
{
    return TRUE;
}
#endif //UNICODE
