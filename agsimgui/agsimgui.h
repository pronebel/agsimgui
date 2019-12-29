#ifndef AGS_IMGUI_H
#define AGS_IMGUI_H

#include "plugin/agsplugin.h"

namespace agsimgui
{
	void AGS_EngineStartup(IAGSEngine *lpEngine);
	void AGS_EngineShutdown();
	int AGS_EngineOnEvent(int event, int data);
	int AGS_EngineDebugHook(const char *scriptName, int lineNum, int reserved);
	void AGS_EngineInitGfx(const char *driverID, void *data);
}

#endif