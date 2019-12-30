// dllmain.cpp : Defines the entry point for the DLL application.

#pragma region Defines_and_Includes

#include "core/platform.h"

#define MIN_EDITOR_VERSION 1
#define MIN_ENGINE_VERSION 3

#if AGS_PLATFORM_OS_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if !defined(BUILTIN_PLUGINS)
#define THIS_IS_THE_PLUGIN
#endif

#include "plugin/agsplugin.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include "imgui/imgui.h" 
#include "imgui/misc/softraster/softraster.h"
#include "imgui/examples/imgui_impl_softraster.h"

#include "agsimgui.h"

#if defined(BUILTIN_PLUGINS)
namespace agsimgui {
#endif

	typedef unsigned char uint8;

#define DEFAULT_RGB_R_SHIFT_32  16
#define DEFAULT_RGB_G_SHIFT_32  8
#define DEFAULT_RGB_B_SHIFT_32  0
#define DEFAULT_RGB_A_SHIFT_32  24

#if !AGS_PLATFORM_OS_WINDOWS
#define min(x,y) (((x) < (y)) ? (x) : (y))
#define max(x,y) (((x) > (y)) ? (x) : (y))
#endif

#define abs(a)             ((a)<0 ? -(a) : (a))

#pragma endregion //Defines_and_Includes

#if AGS_PLATFORM_OS_WINDOWS
	// The standard Windows DLL entry point

	BOOL APIENTRY DllMain(HANDLE hModule,
		DWORD ul_reason_for_call,
		LPVOID lpReserved) {

		switch (ul_reason_for_call) {
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
		}
		return TRUE;
	}
#endif

	//define engine
	IAGSEngine *engine;

#pragma region Color_Functions

	int getr32(int c)
	{
		return ((c >> DEFAULT_RGB_R_SHIFT_32) & 0xFF);
	}


	int getg32(int c)
	{
		return ((c >> DEFAULT_RGB_G_SHIFT_32) & 0xFF);
	}


	int getb32(int c)
	{
		return ((c >> DEFAULT_RGB_B_SHIFT_32) & 0xFF);
	}


	int geta32(int c)
	{
		return ((c >> DEFAULT_RGB_A_SHIFT_32) & 0xFF);
	}


	int makeacol32(int r, int g, int b, int a)
	{
		return ((r << DEFAULT_RGB_R_SHIFT_32) |
			(g << DEFAULT_RGB_G_SHIFT_32) |
			(b << DEFAULT_RGB_B_SHIFT_32) |
			(a << DEFAULT_RGB_A_SHIFT_32));
	}

#pragma endregion



	/// <summary>
	/// Gets the alpha value at coords x,y
	/// </summary>
	int GetAlpha(int sprite, int x, int y) {

		BITMAP *engineSprite = engine->GetSpriteGraphic(sprite);

		unsigned char **charbuffer = engine->GetRawBitmapSurface(engineSprite);
		unsigned int **longbuffer = (unsigned int**)charbuffer;

		int alpha = geta32(longbuffer[y][x]);

		engine->ReleaseBitmapSurface(engineSprite);

		return alpha;
	}

	/// <summary>
	/// Sets the alpha value at coords x,y
	/// </summary>
	int PutAlpha(int sprite, int x, int y, int alpha) {

		BITMAP *engineSprite = engine->GetSpriteGraphic(sprite);

		unsigned char **charbuffer = engine->GetRawBitmapSurface(engineSprite);
		unsigned int **longbuffer = (unsigned int**)charbuffer;


		int r = getr32(longbuffer[y][x]);
		int g = getg32(longbuffer[y][x]);
		int b = getb32(longbuffer[y][x]);
		longbuffer[y][x] = makeacol32(r, g, b, alpha);

		engine->ReleaseBitmapSurface(engineSprite);

		return alpha;
	}


	/// <summary>
	///  Translates index from a 2D array to a 1D array
	/// </summary>
	int xytolocale(int x, int y, int width) {
		return (y * width + x);
	}

	int Clamp(int val, int min, int max) {
		if (val < min) return min;
		else if (val > max) return max;
		else return val;
	}


#if AGS_PLATFORM_OS_WINDOWS

	//==============================================================================

	// ***** Design time *****

	IAGSEditor *editor; // Editor interface

	const char *ourScriptHeader =
" // ags imgui module header \r\n"
"  \r\n"
" enum ImGuiDir \r\n"
" { \r\n"
" eImGuiDir_None    = -1, \r\n"
" eImGuiDir_Left    = 0, \r\n"
" eImGuiDir_Right   = 1, \r\n"
" eImGuiDir_Up      = 2, \r\n"
" eImGuiDir_Down    = 3, \r\n"
" eImGuiDir_COUNT \r\n"
" }; \r\n"
"  \r\n"
" enum ImGuiWindowFlags \r\n"
" { \r\n"
" eImGuiWindowFlags_None                   = 0, \r\n"
" eImGuiWindowFlags_NoTitleBar             = 1,   // Disable title-bar \r\n"
" eImGuiWindowFlags_NoResize               = 2,   // Disable user resizing with the lower-right grip \r\n"
" eImGuiWindowFlags_NoMove                 = 4,   // Disable user moving the window \r\n"
" eImGuiWindowFlags_NoScrollbar            = 8,   // Disable scrollbars (window can still scroll with mouse or programmatically) \r\n"
" eImGuiWindowFlags_NoScrollWithMouse      = 16,   // Disable user vertically scrolling with mouse wheel. On child window, mouse wheel will be forwarded to the parent unless NoScrollbar is also set. \r\n"
" eImGuiWindowFlags_NoCollapse             = 32,   // Disable user collapsing window by double-clicking on it \r\n"
" eImGuiWindowFlags_AlwaysAutoResize       = 64,   // Resize every window to its content every frame \r\n"
" eImGuiWindowFlags_NoBackground           = 128,   // Disable drawing background color (WindowBg, etc.) and outside border. Similar as using SetNextWindowBgAlpha(0.0f). \r\n"
" eImGuiWindowFlags_NoSavedSettings        = 256,   // Never load/save settings in .ini file \r\n"
" eImGuiWindowFlags_NoMouseInputs          = 512,   // Disable catching mouse, hovering test with pass through. \r\n"
" eImGuiWindowFlags_MenuBar                = 1024,  // Has a menu-bar \r\n"
" eImGuiWindowFlags_HorizontalScrollbar    = 2048,  // Allow horizontal scrollbar to appear (off by default). \r\n"
" eImGuiWindowFlags_NoFocusOnAppearing     = 4096,  // Disable taking focus when transitioning from hidden to visible state \r\n"
" eImGuiWindowFlags_NoBringToFrontOnFocus  = 8192,  // Disable bringing window to front when taking focus (e.g. clicking on it or programmatically giving it focus) \r\n"
" eImGuiWindowFlags_AlwaysVerticalScrollbar= 16384,  // Always show vertical scrollbar (even if ContentSize.y < Size.y) \r\n"
" eImGuiWindowFlags_AlwaysHorizontalScrollbar= 32768,  // Always show horizontal scrollbar (even if ContentSize.x < Size.x) \r\n"
" eImGuiWindowFlags_AlwaysUseWindowPadding = 65536,  // Ensure child windows without border uses style.WindowPadding (ignored by default for non-bordered child windows, because more convenient) \r\n"
" eImGuiWindowFlags_NoNavInputs            = 262144,  // No gamepad/keyboard navigation within the window \r\n"
" eImGuiWindowFlags_NoNavFocus             = 524288,  // No focusing toward this window with gamepad/keyboard navigation (e.g. skipped by CTRL+TAB) \r\n"
" eImGuiWindowFlags_UnsavedDocument        = 1048576,  // Append '*' to title without affecting the ID, as a convenience to avoid using the ### operator. \r\n"
" eImGuiWindowFlags_NoNav                  = 786432, \r\n"
" eImGuiWindowFlags_NoDecoration           = 43, \r\n"
" eImGuiWindowFlags_NoInputs               = 786944, \r\n"
" }; \r\n"
"  \r\n"
" enum ImGuiInputTextFlags \r\n"
" { \r\n"
" eImGuiInputTextFlags_None                = 0, \r\n"
" eImGuiInputTextFlags_CharsDecimal        = 1,   // Allow 0123456789.+-*/ \r\n"
" eImGuiInputTextFlags_CharsHexadecimal    = 2,   // Allow 0123456789ABCDEFabcdef \r\n"
" eImGuiInputTextFlags_CharsUppercase      = 4,   // Turn a..z into A..Z \r\n"
" eImGuiInputTextFlags_CharsNoBlank        = 8,   // Filter out spaces, tabs \r\n"
" eImGuiInputTextFlags_AutoSelectAll       = 16,   // Select entire text when first taking mouse focus \r\n"
" eImGuiInputTextFlags_EnterReturnsTrue    = 32,   // Return 'true' when Enter is pressed (as opposed to every time the value was modified). \r\n"
" eImGuiInputTextFlags_CallbackCompletion  = 64,   // Callback on pressing TAB (for completion handling) \r\n"
" eImGuiInputTextFlags_CallbackHistory     = 128,   // Callback on pressing Up/Down arrows (for history handling) \r\n"
" eImGuiInputTextFlags_CallbackAlways      = 256,   // Callback on each iteration. User code may query cursor position, modify text buffer. \r\n"
" eImGuiInputTextFlags_CallbackCharFilter  = 512,   // Callback on character inputs to replace or discard them. Modify 'EventChar' to replace or discard, or return 1 in callback to discard. \r\n"
" eImGuiInputTextFlags_AllowTabInput       = 1024,  // Pressing TAB input a 't' character into the text field \r\n"
" eImGuiInputTextFlags_CtrlEnterForNewLine = 2048,  // In multi-line mode, unfocus with Enter, add new line with Ctrl+Enter (default is opposite: unfocus with Ctrl+Enter, add line with Enter). \r\n"
" eImGuiInputTextFlags_NoHorizontalScroll  = 4096,  // Disable following the cursor horizontally \r\n"
" eImGuiInputTextFlags_AlwaysInsertMode    = 8192,  // Insert mode \r\n"
" eImGuiInputTextFlags_ReadOnly            = 16384,  // Read-only mode \r\n"
" eImGuiInputTextFlags_Password            = 32768,  // Password mode, display all characters as '*' \r\n"
" eImGuiInputTextFlags_NoUndoRedo          = 65536,  // Disable undo/redo. Note that input text owns the text data while active, if you want to provide your own undo/redo stack you need e.g. to call ClearActiveID(). \r\n"
" eImGuiInputTextFlags_CharsScientific     = 131072,  // Allow 0123456789.+-*/eE (Scientific notation input) \r\n"
" eImGuiInputTextFlags_CallbackResize      = 262144,  // Callback on buffer capacity changes request (beyond 'buf_size' parameter value), allowing the string to grow. \r\n"
" }; \r\n"
"  \r\n"
" struct agsimgui{ \r\n"
" // Main \r\n"
"  \r\n"
" /// start a new Dear ImGui frame, you can submit any command from this point until Render()/EndFrame(). \r\n"
" import static void NewFrame(); \r\n"
"  \r\n"
" /// ends the Dear ImGui frame. automatically called by Render(), you likely don't need to call that yourself directly. \r\n"
" import static void EndFrame(); \r\n"
"  \r\n"
" /// ends the Dear ImGui frame, finalize the draw data. You can get call GetDrawData() to obtain it and run your rendering function. \r\n"
" import static void Render(); \r\n"
"  \r\n"
" /// valid after Render() and until the next call to NewFrame(). this is what you have to render. \r\n"
" import static int GetDrawData(); \r\n"
"  \r\n"
"  \r\n"
" // Demo, Debug, Information \r\n"
"  \r\n"
" /// get the compiled version string e.g. \"1.23\" (essentially the compiled value for IMGUI_VERSION) \r\n"
" import static String GetVersion(); \r\n"
"  \r\n"
"  \r\n"
" // Windows \r\n"
"  \r\n"
" /// Push window to the stack. Always call a matching End() for each Begin() call. Return false to indicate the window is collapsed or fully clipped \r\n"
" import static bool Begin(String name, ImGuiWindowFlags flags = 0); \r\n"
"  \r\n"
" /// pop window from the stack. \r\n"
" import static void End(); \r\n"
"  \r\n"
" // Child Windows \r\n"
"  \r\n"
" /// Child Windows. Always call a matching EndChild() for each BeginChild() call, regardless of its return value. Child windows can embed their own child. \r\n"
" import static bool BeginChild(String str_id, int width = 0, int height = 0, bool border = false, ImGuiWindowFlags flags = 0); \r\n"
"  \r\n"
" /// pop child window from the stack. \r\n"
" import static void EndChild(); \r\n"
"  \r\n"
"  \r\n"
" // Widgets: Text \r\n"
"  \r\n"
" /// draws text \r\n"
" import static void          Text(String text); \r\n"
"  \r\n"
" /// shortcut for PushStyleColor(ImGuiCol_Text, col); Text(fmt, ...); PopStyleColor(); \r\n"
" import static void          TextColored(int color, String text); \r\n"
"  \r\n"
" /// shortcut for PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]); Text(fmt, ...); PopStyleColor(); \r\n"
" import static void          TextDisabled(String text); \r\n"
"  \r\n"
" /// shortcut for PushTextWrapPos(0.0f); Text(fmt, ...); PopTextWrapPos();. Note that this won't work on an auto- \r\n"
" import static void          TextWrapped(String text); \r\n"
"  \r\n"
" /// display text+label aligned the same way as value+label widgets \r\n"
" import static void          LabelText(String label, String text); \r\n"
"  \r\n"
" /// shortcut for Bullet()+Text() \r\n"
" import static void          BulletText(String text); \r\n"
"  \r\n"
"  \r\n"
" // Widgets: Main \r\n"
" // - Most widgets return true when the value has been changed or when pressed/selected \r\n"
" // - You may also use one of the many IsItemXXX functions (e.g. IsItemActive, IsItemHovered, etc.) to query widget state. \r\n"
"  \r\n"
" /// button \r\n"
" import static bool Button(String label, int width, int height); \r\n"
"  \r\n"
" /// button with FramePadding=(0,0) to easily embed within text \r\n"
" import static bool SmallButton(String label); \r\n"
"  \r\n"
" /// square button with an arrow shape \r\n"
" import static bool ArrowButton(String str_id, ImGuiDir dir); \r\n"
"  \r\n"
" import static bool Checkbox(String label, bool v); \r\n"
"  \r\n"
" /// use with e.g. if (RadioButton(\"one\", my_value==1)) { my_value = 1; } \r\n"
" import static bool RadioButton(String label, bool active); \r\n"
"  \r\n"
" /// draw a small circle and keep the cursor on the same line. advance cursor x position by GetTreeNodeToLabelSpacing(), same distance that TreeNode() uses \r\n"
" import static  void Bullet(); \r\n"
" }; \r\n";


	//------------------------------------------------------------------------------
	LPCSTR AGS_GetPluginName(void)
	{
		// Return the plugin description
		return "agsimgui";
	}

	//------------------------------------------------------------------------------

	int AGS_EditorStartup(IAGSEditor *lpEditor)
	{
		// User has checked the plugin to use it in their game

		// If it's an earlier version than what we need, abort.
		if (lpEditor->version < MIN_EDITOR_VERSION)
			return (-1);

		editor = lpEditor;
		editor->RegisterScriptHeader(ourScriptHeader);

		// Return 0 to indicate success
		return (0);
	}

	//------------------------------------------------------------------------------

	void AGS_EditorShutdown()
	{
		// User has un-checked the plugin from their game
		editor->UnregisterScriptHeader(ourScriptHeader);
	}

	//------------------------------------------------------------------------------

	void AGS_EditorProperties(HWND parent)                        //*** optional ***
	{
		// User has chosen to view the Properties of the plugin
		// We could load up an options dialog or something here instead
/*	MessageBox(parent,
			 L"agsimgui v0.1.0 By eri0o",
			 L"About",
		 MB_OK | MB_ICONINFORMATION);
 */
	}

	//------------------------------------------------------------------------------

	int AGS_EditorSaveGame(char *buffer, int bufsize)             //*** optional ***
	{
		// Called by the editor when the current game is saved to disk.
		// Plugin configuration can be stored in [buffer] (max [bufsize] bytes)
		// Return the amount of bytes written in the buffer
		return (0);
	}

	//------------------------------------------------------------------------------

	void AGS_EditorLoadGame(char *buffer, int bufsize)            //*** optional ***
	{
		// Called by the editor when a game is loaded from disk
		// Previous written data can be read from [buffer] (size [bufsize]).
		// Make a copy of the data, the buffer is freed after this function call.
	}

	//==============================================================================

#endif

// ***** Run time *****

ImVec4 FromAgsColors(int color){
    return ImVec4(
           ((float) getr32(color))/255.0,
           ((float) getg32(color))/255.0,
           ((float) getb32(color))/255.0,
           ((float) geta32(color))/255.0);
}



// Engine interface

//------------------------------------------------------------------------------

#define STRINGIFY(s) STRINGIFY_X(s)
#define STRINGIFY_X(s) #s

texture_color32_t screen;
ImGuiContext *context;

void AgsImGui_NewFrame(){
    ImGui_ImplSoftraster_NewFrame();
    ImGui::NewFrame();
}

void AgsImGui_EndFrame(){
    ImGui::EndFrame();
}

void AgsImGui_Render(){
    ImGui::Render();
    ImGui_ImplSoftraster_RenderDrawData(ImGui::GetDrawData());
}

int AgsImGui_GetDrawData(){
    return ImGui_ImplSoftraster_GetSprite();
}

const char* AgsImGui_GetVersion(){
    return ImGui::GetVersion();
}

bool AgsImGui_Begin(const char* name, int32 flags = 0){
    return ImGui::Begin(name, NULL, flags);
}

void AgsImGui_End(){
    ImGui::End();
}

bool AgsImGui_BeginChild(const char* str_id, int width = 0, int height = 0, bool border = false, int32 flags = 0){
    return ImGui::BeginChild(str_id,ImVec2((float) width,(float) height), border, flags);
}

void AgsImGui_EndChild(){
    ImGui::EndChild();
}

void AgsImGui_Text(const char* text){
    ImGui::Text(text);
}

void AgsImGui_TextColored(int color, const char* text){
    ImGui::TextColored(FromAgsColors(color), text);
}

void AgsImGui_TextDisabled(const char* text){
    ImGui::TextDisabled(text);
}

void AgsImGui_TextWrapped(const char* text){
    ImGui::TextWrapped(text);
}

void AgsImGui_LabelText(const char* label, const char* text){
    ImGui::LabelText(label, text);
}

void AgsImGui_BulletText(const char* text){
    ImGui::BulletText(text);
}

bool AgsImGui_Button(const char* label, int width, int height){
    return ImGui::Button(label, ImVec2((float) width, (float) height));
}

bool AgsImGui_SmallButton(const char* label){
    return ImGui::SmallButton(label);
}

bool AgsImGui_ArrowButton(const char* str_id, int32 dir){
    return ImGui::ArrowButton(str_id, dir);
}

bool AgsImGui_Checkbox(const char* label, bool v){
    return ImGui::Checkbox(label, &v);
}

bool AgsImGui_RadioButton(const char* label, bool active){
    return ImGui::RadioButton(label, active);
}

void AgsImGui_Bullet(){
    ImGui::Bullet();
}



	void AGS_EngineStartup(IAGSEngine *lpEngine)
	{
		engine = lpEngine;

		// Make sure it's got the version with the features we need
		if (engine->version < MIN_ENGINE_VERSION)
			engine->AbortGame("Plugin needs engine version " STRINGIFY(MIN_ENGINE_VERSION) " or newer.");

		//register functions

        context = ImGui::CreateContext();
        ImGui_ImplSoftraster_Init(&screen);

        engine->RegisterScriptFunction("agsimgui::NewFrame^0", (void*)AgsImGui_NewFrame);
        engine->RegisterScriptFunction("agsimgui::EndFrame^0", (void*)AgsImGui_EndFrame);
        engine->RegisterScriptFunction("agsimgui::Render^0", (void*)AgsImGui_Render);
        engine->RegisterScriptFunction("agsimgui::GetDrawData^0", (void*)AgsImGui_GetDrawData);
        engine->RegisterScriptFunction("agsimgui::GetVersion^0", (void*)AgsImGui_GetVersion);
        engine->RegisterScriptFunction("agsimgui::Begin^2", (void*)AgsImGui_Begin);
        engine->RegisterScriptFunction("agsimgui::End^0", (void*)AgsImGui_End);
        engine->RegisterScriptFunction("agsimgui::BeginChild^5", (void*)AgsImGui_BeginChild);
        engine->RegisterScriptFunction("agsimgui::EndChild^0", (void*)AgsImGui_EndChild);
        engine->RegisterScriptFunction("agsimgui::Text^1", (void*)AgsImGui_Text);
        engine->RegisterScriptFunction("agsimgui::TextColored^2", (void*)AgsImGui_TextColored);
        engine->RegisterScriptFunction("agsimgui::TextDisabled^1", (void*)AgsImGui_TextDisabled);
        engine->RegisterScriptFunction("agsimgui::TextWrapped^1", (void*)AgsImGui_TextWrapped);
        engine->RegisterScriptFunction("agsimgui::LabelText^2", (void*)AgsImGui_LabelText);
        engine->RegisterScriptFunction("agsimgui::BulletText^1", (void*)AgsImGui_BulletText);
        engine->RegisterScriptFunction("agsimgui::Button^3", (void*)AgsImGui_Button);
        engine->RegisterScriptFunction("agsimgui::SmallButton^1", (void*)AgsImGui_SmallButton);
        engine->RegisterScriptFunction("agsimgui::ArrowButton^2", (void*)AgsImGui_ArrowButton);
        engine->RegisterScriptFunction("agsimgui::Checkbox^2", (void*)AgsImGui_Checkbox);
        engine->RegisterScriptFunction("agsimgui::RadioButton^2", (void*)AgsImGui_RadioButton);
        engine->RegisterScriptFunction("agsimgui::Bullet^0", (void*)AgsImGui_Bullet);


	}

	//------------------------------------------------------------------------------

	void AGS_EngineShutdown()
	{
		// Called by the game engine just before it exits.
		// This gives you a chance to free any memory and do any cleanup
		// that you need to do before the engine shuts down.
	}

	//------------------------------------------------------------------------------

    bool do_only_once = false;
    int AGS_EngineOnEvent(int event, int data)                    //*** optional ***
    {
        if(event==AGSE_PRESCREENDRAW){
            //initialize debug
            if(!do_only_once) {
                int screenWidth, screenHeight, colDepth;
                engine->GetScreenDimensions(&screenWidth, &screenHeight, &colDepth);
                printf("\nagsimgui 0.1.0\n");
                ImGui_ImplSoftraster_InitializeScreenAgs(engine,screenWidth, screenHeight, colDepth);
                screen.init(screenWidth, screenHeight);
                do_only_once = true;
            }
        }

		switch (event)
		{
			/*
				case AGSE_KEYPRESS:
				case AGSE_MOUSECLICK:
				case AGSE_POSTSCREENDRAW:
				case AGSE_PRESCREENDRAW:
				case AGSE_SAVEGAME:
				case AGSE_RESTOREGAME:
				case AGSE_PREGUIDRAW:
				case AGSE_LEAVEROOM:
				case AGSE_ENTERROOM:
				case AGSE_TRANSITIONIN:
				case AGSE_TRANSITIONOUT:
				case AGSE_FINALSCREENDRAW:
				case AGSE_TRANSLATETEXT:
				case AGSE_SCRIPTDEBUG:
				case AGSE_SPRITELOAD:
				case AGSE_PRERENDER:
				case AGSE_PRESAVEGAME:
				case AGSE_POSTRESTOREGAME:
			 */
		default:
			break;
		}

		// Return 1 to stop event from processing further (when needed)
		return (0);
	}

	//------------------------------------------------------------------------------

	int AGS_EngineDebugHook(const char *scriptName,
		int lineNum, int reserved)            //*** optional ***
	{
		// Can be used to debug scripts, see documentation
		return 0;
	}

	//------------------------------------------------------------------------------

	void AGS_EngineInitGfx(const char *driverID, void *data)      //*** optional ***
	{
		// This allows you to make changes to how the graphics driver starts up.
		// See documentation
	}

	//..............................................................................


#if defined(BUILTIN_PLUGINS)
}
#endif
