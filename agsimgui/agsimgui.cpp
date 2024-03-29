// dllmain.cpp : Defines the entry point for the DLL application.

#pragma region Defines_and_Includes

#include "core/platform.h"

#define MIN_EDITOR_VERSION 1
#define MIN_ENGINE_VERSION 3

#if AGS_PLATFORM_OS_WINDOWS

struct IUnknown; // Workaround for "combaseapi.h(229): error C2187: syntax error: 'identifier' was unexpected here" when using /permissive-s
#include <d3d9.h>
#include <d3dx9.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#if !defined(BUILTIN_PLUGINS)
#define THIS_IS_THE_PLUGIN
#endif

#include "ac/keycode.h"
#include "plugin/agsplugin.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <vector>
#include "imgui/imgui.h" 
#include "imgui/misc/softraster/softraster.h"
#include "imgui/examples/imgui_impl_softraster.h"
#include "imgui/examples/imgui_impl_dx9.h"

#include "imgui/misc/cpp/imgui_stdlib.h"

#include "agsimgui.h"
#include "Screen.h"
#include "libs/clip/clip.h"

#include <cstring>

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
" enum ImGuiFocusedFlags \r\n"
" { \r\n"
"   ImGuiFocusedFlags_None = 0, \r\n"
"   ImGuiFocusedFlags_ChildWindows = 1,   // IsWindowFocused(): Return true if any children of the window is focused \r\n"
"   ImGuiFocusedFlags_RootWindow = 2,   // IsWindowFocused(): Test from root window (top most parent of the current hierarchy) \r\n"
"   ImGuiFocusedFlags_AnyWindow = 4,   // IsWindowFocused(): Return true if any window is focused.  \r\n"
"   ImGuiFocusedFlags_RootAndChildWindows  = 3 \r\n"
" }; \r\n"
"  \r\n"
" enum ImGuiHoveredFlags \r\n"
" { \r\n"
"   eImGuiHoveredFlags_None                          = 0,  // Return true if directly over the item/window, not obstructed by another window, not obstructed by an active popup or modal blocking inputs under them. \r\n"
"   eImGuiHoveredFlags_ChildWindows                  = 1,  // IsWindowHovered() only: Return true if any children of the window is hovered \r\n"
"   eImGuiHoveredFlags_RootWindow                    = 2,  // IsWindowHovered() only: Test from root window (top most parent of the current hierarchy) \r\n"
"   eImGuiHoveredFlags_AnyWindow                     = 4,  // IsWindowHovered() only: Return true if any window is hovered \r\n"
"   eImGuiHoveredFlags_AllowWhenBlockedByPopup       = 8,  // Return true even if a popup window is normally blocking access to this item/window \r\n"
"   eImGuiHoveredFlags_AllowWhenBlockedByActiveItem  = 32, // Return true even if an active item is blocking access to this item/window. Useful for Drag and Drop patterns. \r\n"
"   eImGuiHoveredFlags_AllowWhenOverlapped           = 64, // Return true even if the position is obstructed or overlapped by another window \r\n"
"   eImGuiHoveredFlags_AllowWhenDisabled             = 128, // Return true even if the item is disabled \r\n"
"   eImGuiHoveredFlags_RectOnly                      = 104, \r\n"
"   eImGuiHoveredFlags_RootAndChildWindows           = 3 \r\n"
" }; \r\n"
"  \r\n"
" enum ImGuiTabBarFlags \r\n"
" { \r\n"
"   eImGuiTabBarFlags_None                           = 0, \r\n"
"   eImGuiTabBarFlags_Reorderable                    = 1,   // Allow manually dragging tabs to re-order them + New tabs are appended at the end of list \r\n"
"   eImGuiTabBarFlags_AutoSelectNewTabs              = 2,   // Automatically select new tabs when they appear \r\n"
"   eImGuiTabBarFlags_TabListPopupButton             = 4,   // Disable buttons to open the tab list popup \r\n"
"   eImGuiTabBarFlags_NoCloseWithMiddleMouseButton   = 8,   // Disable behavior of closing tabs (that are submitted with p_open != NULL) with middle mouse button.  \r\n"
"   eImGuiTabBarFlags_NoTabListScrollingButtons      = 16,   // Disable scrolling buttons (apply when fitting policy is ImGuiTabBarFlags_FittingPolicyScroll) \r\n"
"   eImGuiTabBarFlags_NoTooltip                      = 32,   // Disable tooltips when hovering a tab \r\n"
"   eImGuiTabBarFlags_FittingPolicyResizeDown        = 64,   // Resize tabs when they don't fit \r\n"
"   eImGuiTabBarFlags_FittingPolicyScroll            = 128,   // Add scroll buttons when tabs don't fit \r\n"
"   eImGuiTabBarFlags_FittingPolicyMask_             = 192, \r\n"
"   eImGuiTabBarFlags_FittingPolicyDefault_          = 64 \r\n"
" }; \r\n"
"  \r\n"
" enum ImGuiTabItemFlags \r\n"
" { \r\n"
"   ImGuiTabItemFlags_None = 0, \r\n"
"   ImGuiTabItemFlags_UnsavedDocument = 1,   // Append '*' to title without affecting the ID. \r\n"
"   ImGuiTabItemFlags_SetSelected = 2,   // Trigger flag to programmatically make the tab selected when calling BeginTabItem() \r\n"
"   ImGuiTabItemFlags_NoCloseWithMiddleMouseButton = 4,   // Disable behavior of closing tabs  \r\n"
"   ImGuiTabItemFlags_NoPushId = 8    // Don't call PushID(tab->ID)/PopID() on BeginTabItem()/EndTabItem() \r\n"
" }; \r\n"
"  \r\n"
" enum ImGuiBeginWindow \r\n"
" { \r\n"
"    eImGuiBeginWindow_Fail = 0, \r\n"
"    eImGuiBeginWindow_OK = 1, \r\n"
"    eImGuiBeginWindow_Collapsed = 2, \r\n"
"    eImGuiBeginWindow_OK_Closed = 3, \r\n"
"    eImGuiBeginWindow_Collapsed_Closed = 4, \r\n"
" }; \r\n"
"  \r\n"
" enum ImGuiSelectableFlags \r\n"
" { \r\n"
"    eImGuiSelectableFlags_None               = 0, \r\n"
"    eImGuiSelectableFlags_DontClosePopups    = 1,  // Clicking this don't close parent popup window \r\n"
"    eImGuiSelectableFlags_SpanAllColumns     = 2,  // Selectable frame can span all columns (text will still fit in current column) \r\n"
"    eImGuiSelectableFlags_AllowDoubleClick   = 4,  // Generate press events on double clicks too \r\n"
"    eImGuiSelectableFlags_Disabled           = 8,  // Cannot be selected, display grayed out text \r\n"
"    eImGuiSelectableFlags_AllowItemOverlap   = 16  // (WIP) Hit testing to allow subsequent widgets to overlap this one \r\n"
" }; \r\n"
"  \r\n"
" enum ImGuiComboFlags \r\n"
" { \r\n"
"   eImGuiComboFlags_None = 0, \r\n"
"   eImGuiComboFlags_PopupAlignLeft = 1,  // Align the popup toward the left by default \r\n"
"   eImGuiComboFlags_HeightSmall    = 2,  // Max ~4 items visible.  \r\n"
"   eImGuiComboFlags_HeightRegular  = 4,  // Max ~8 items visible (default) \r\n"
"   eImGuiComboFlags_HeightLarge    = 8,  // Max ~20 items visible \r\n"
"   eImGuiComboFlags_HeightLargest  = 16, // As many fitting items as possible \r\n"
"   eImGuiComboFlags_NoArrowButton  = 32, // Display on the preview box without the square arrow button \r\n"
"   eImGuiComboFlags_NoPreview      = 64, // Display only a square arrow button \r\n"
"   eImGuiComboFlags_HeightMask_    = 30, \r\n"
" }; \r\n"
" enum ImGuiDir \r\n"
" { \r\n"
" eImGuiDir_None = -1, \r\n"
" eImGuiDir_Left = 0, \r\n"
" eImGuiDir_Right = 1, \r\n"
" eImGuiDir_Up = 2, \r\n"
" eImGuiDir_Down = 3, \r\n"
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
" enum ImGuiCond \r\n"
" { \r\n"
"   ImGuiCond_Always       = 1, // Set the variable \r\n"
"   ImGuiCond_Once         = 2, // Set the variable once per runtime session (only the first call with succeed) \r\n"
"   ImGuiCond_FirstUseEver = 4, // Set the variable if the object/window has no persistently saved data (no entry in .ini file) \r\n"
"   ImGuiCond_Appearing    = 8, // Set the variable if the object/window is appearing after being hidden/inactive (or the first time) \r\n"
" }; \r\n"
"  \r\n"
" struct AgsImGui{ \r\n"
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
" // valid after Render() and until the next call to NewFrame(). this is what you have to render. \r\n"
" //import static int GetDrawData(); \r\n"
"  \r\n"
"  \r\n"
" // Demo, Debug, Information \r\n"
"  \r\n"
" /// create Demo window (previously called ShowTestWindow). demonstrate most ImGui features. call this to learn about the library! \r\n"
" import static void ShowDemoWindow(); \r\n"
"  \r\n"
" /// create About window. display Dear ImGui version, credits and build/system information. \r\n"
" import static void ShowAboutWindow(); \r\n"
"  \r\n"
" /// create Metrics/Debug window. display Dear ImGui internals: draw commands (with individual draw calls and vertices), window list, basic internal state, etc. \r\n"
" import static void ShowMetricsWindow(); \r\n"
"  \r\n"
" /// add basic help/info block (not a window): how to manipulate ImGui as a end-user (mouse/keyboard controls). \r\n"
" import static void ShowUserGuide(); \r\n"
"  \r\n"
" /// get the compiled version string e.g. \"1.23\" (essentially the compiled value for IMGUI_VERSION) \r\n"
" import static String GetVersion(); \r\n"
"  \r\n"
" import static void StyleColorsDark(); \r\n"
"  \r\n"
" import static void StyleColorsClassic(); \r\n"
"  \r\n"
" import static void StyleColorsLight(); \r\n"
"  \r\n"
" // Windows \r\n"
"  \r\n"
" /// Push window to the stack. Always call a matching End() for each Begin() call. Return value indicates if it's collapsed or if clicked to close. \r\n"
" import static ImGuiBeginWindow BeginWindow(String name, bool has_close_button = 0, ImGuiWindowFlags flags = 0); \r\n"
"  \r\n"
" /// pop window from the stack. \r\n"
" import static void EndWindow(); \r\n"
"  \r\n"
" // Child Windows \r\n"
"  \r\n"
" /// Child Windows. Always call a matching EndChild() for each BeginChild() call, regardless of its return value. Child windows can embed their own child. \r\n"
" import static bool BeginChild(String str_id, int width = 0, int height = 0, bool border = false, ImGuiWindowFlags flags = 0); \r\n"
"  \r\n"
" /// pop child window from the stack. \r\n"
" import static void EndChild(); \r\n"
"  \r\n"
" /// set next window position. call before Begin(). use pivot=(0.5,0.5) to center on given point, etc. \r\n"
" import static void SetNextWindowPos(int position_x, int position_y, ImGuiCond cond = 0, float pivot_x = 0, float pivot_y = 0); \r\n"
"  \r\n"
" /// set next window size. set axis to 0 to force an auto-fit on this axis. call before Begin() \r\n"
" import static void SetNextWindowSize(int width = 0, int height = 0, ImGuiCond cond = 0); \r\n"
"  \r\n"
" /// set next window size limits. use -1,-1 on either X/Y axis to preserve the current size. Sizes will be rounded down. \r\n"
" import static void SetNextWindowSizeConstraints(int min_width, int min_height, int max_width, int max_height); \r\n"
"  \r\n"
" /// set next window content size (~ scrollable client area, which enforce the range of scrollbars). Not including window decorations nor WindowPadding. set an axis to 0 to leave it automatic. call before Begin() \r\n"
" import static void SetNextWindowContentSize(int width = 0, int height = 0); \r\n"
"  \r\n"
" /// set next window collapsed state. call before Begin() \r\n"
" import static void SetNextWindowCollapsed(bool collapsed, ImGuiCond cond = 0); \r\n"
"  \r\n"
" /// set next window to be focused / top-most. call before Begin() \r\n"
" import static void SetNextWindowFocus(); \r\n"
"  \r\n"
" /// set next window background color alpha. helper to easily modify ImGuiCol_WindowBg/ChildBg/PopupBg. \r\n"
" import static void SetNextWindowBgAlpha(float alpha); \r\n"
"  \r\n"
" // Item/Widgets Utilities \r\n"
" // - Most of the functions are referring to the last/previous item we submitted. \r\n"
"  \r\n"
" /// is the last item hovered? (and usable, aka not blocked by a popup, etc.). See ImGuiHoveredFlags for more options. \r\n"
" import static bool IsItemHovered(ImGuiHoveredFlags flags = 0);  \r\n"
"  \r\n"
" /// is the last item active? (e.g. button being held, text field being edited. This will continuously return true while holding mouse button on an item.) \r\n"
" import static bool IsItemActive();  \r\n"
"  \r\n"
" /// is the last item focused for keyboard/gamepad navigation? \r\n"
" import static bool IsItemFocused(); \r\n"
"  \r\n"
" /// is the last item visible? (items may be out of sight because of clipping/scrolling) \r\n"
" import static bool IsItemVisible(); \r\n"
"  \r\n"
" /// did the last item modify its underlying value this frame? or was pressed? This is generally the same as the bool return value of many widgets. \r\n"
" import static bool IsItemEdited(); \r\n"
"  \r\n"
" /// was the last item just made active (item was previously inactive). \r\n"
" import static bool IsItemActivated(); \r\n"
"  \r\n"
" /// was the last item just made inactive (item was previously active). Useful for Undo/Redo patterns with widgets that requires continuous editing. \r\n"
" import static bool IsItemDeactivated(); \r\n"
"  \r\n"
" /// was the last item just made inactive and made a value change when it was active? (e.g. Slider/Drag moved). Useful for Undo/Redo patterns with widgets that requires continuous editing. \r\n"
" import static bool IsItemDeactivatedAfterEdit(); \r\n"
"  \r\n"
" /// was the last item open state toggled? set by TreeNode(). \r\n"
" import static bool IsItemToggledOpen(); \r\n"
"  \r\n"
" /// is any item hovered? \r\n"
" import static bool IsAnyItemHovered();  \r\n"
"  \r\n"
" /// is any item active? \r\n"
" import static bool IsAnyItemActive(); \r\n"
"  \r\n"
" /// is any item focused? \r\n"
" import static bool IsAnyItemFocused();  \r\n"
"  \r\n"
" // Windows Utilities \r\n"
" // - 'current window' = the window we are appending into while inside a Begin()/End() block.  \r\n"
" import static bool IsWindowAppearing(); \r\n"
"  \r\n"
" /// return true when window is collapsed. Use this between Begin and End of a window. \r\n"
" import static bool IsWindowCollapsed(); \r\n"
"  \r\n"
" /// is current window focused? or its root/child, depending on flags. see flags for options. Use this between Begin and End of a window. \r\n"
" import static bool IsWindowFocused(ImGuiFocusedFlags flags=0); \r\n"
"  \r\n"
" /// is current window hovered (and typically: not blocked by a popup/modal)? see flags for options. Use this between Begin and End of a window.\r\n"
" import static bool IsWindowHovered(ImGuiHoveredFlags flags=0); \r\n"
"  \r\n"
" // Layout \r\n"
" // - By cursor we mean the current output position. \r\n"
" // - The typical widget behavior is to output themselves at the current cursor position, then move the cursor one line down. \r\n"
" // - You can call SameLine() between widgets to undo the last carriage return and output at the right of the preceeding widget. \r\n"
"  \r\n"
" /// separator, generally horizontal. inside a menu bar or in horizontal layout mode, this becomes a vertical separator. \r\n"
" import static void Separator(); \r\n"
"  \r\n"
" /// call between widgets or groups to layout them horizontally. X position given in window coordinates. \r\n"
" import static void SameLine(float offset_from_start_x = 0, float spacing=0); \r\n"
"  \r\n"
" /// undo a SameLine() or force a new line when in an horizontal-layout context. \r\n"
" import static void NewLine(); \r\n"
"  \r\n"
" /// add vertical spacing. \r\n"
" import static void Spacing(); \r\n"
"  \r\n"
" /// add a dummy item of given size. unlike InvisibleButton(), Dummy() won't take the mouse click or be navigable into. \r\n"
" import static void Dummy(float width, float height); \r\n"
"  \r\n"
" /// move content position toward the right, by style.IndentSpacing or indent_w if != 0 \r\n"
" import static void Indent(float indent_w = 0); \r\n"
"  \r\n"
" /// move content position back to the left, by style.IndentSpacing or indent_w if != 0 \r\n"
" import static void Unindent(float indent_w = 0); \r\n"
"  \r\n"
" // ID stack/scopes \r\n"
" // - Read the FAQ for more details about how ID are handled in dear imgui. If you are creating widgets in a loop you most \r\n"
" //   likely want to push a unique identifier (e.g. object pointer, loop index) to uniquely differentiate them. \r\n"
" // - The resulting ID are hashes of the entire stack. \r\n"
" // - You can also use the `Label##foobar` syntax within widget label to distinguish them from each others. \r\n"
" // - In this header file we use the `label`/`name` terminology to denote a string that will be displayed and used as an ID, \r\n"
" //   whereas `str_id` denote a string that is only used as an ID and not normally displayed. \r\n"
"  \r\n"
" /// push string into the ID stack (will hash string). \r\n"
" import static void PushID(String str_id); \r\n"
"  \r\n"
" /// pop from the ID stack. \r\n"
" import static void PopID(); \r\n"
"  \r\n"
" // Widgets: Text \r\n"
"  \r\n"
" /// draws text \r\n"
" import static void Text(String text); \r\n"
"  \r\n"
" /// shortcut for PushStyleColor(ImGuiCol_Text, col); Text(fmt, ...); PopStyleColor(); \r\n"
" import static void TextColored(int color, String text); \r\n"
"  \r\n"
" /// shortcut for PushStyleColor(ImGuiCol_Text, style.Colors[ImGuiCol_TextDisabled]); Text(fmt, ...); PopStyleColor(); \r\n"
" import static void TextDisabled(String text); \r\n"
"  \r\n"
" /// shortcut for PushTextWrapPos(0.0f); Text(fmt, ...); PopTextWrapPos();. Note that this won't work on an auto- \r\n"
" import static void TextWrapped(String text); \r\n"
"  \r\n"
" /// display text+label aligned the same way as value+label widgets \r\n"
" import static void LabelText(String label, String text); \r\n"
"  \r\n"
" /// shortcut for Bullet()+Text() \r\n"
" import static void BulletText(String text); \r\n"
"  \r\n"
"  \r\n"
" // Widgets: Main \r\n"
" // - Most widgets return true when the value has been changed or when pressed/selected \r\n"
" // - You may also use one of the many IsItemXXX functions (e.g. IsItemActive, IsItemHovered, etc.) to query widget state. \r\n"
"  \r\n"
" /// button \r\n"
" import static bool Button(String label, int width = 0, int height = 0); \r\n"
"  \r\n"
" /// button with FramePadding=(0,0) to easily embed within text \r\n"
" import static bool SmallButton(String label); \r\n"
"  \r\n"
" /// create an image with passed sprite ID. \r\n"
" import static void Image(int sprite_id); \r\n"
"  \r\n"
" /// create a button with an image with passed sprite ID. Returns true while clicked. \r\n"
" import static bool ImageButton(int sprite_id); \r\n"
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
" import static void Bullet(); \r\n"
"  \r\n"
" // Widgets: Selectables \r\n"
" // - A selectable highlights when hovered, and can display another color when selected. \r\n"
"  \r\n"
" /// bool selected carry the selection state (read-only). When Selectable() is clicked it returns true so you can modify your selection state. \r\n"
" import static bool Selectable(String label, bool selected = false, ImGuiSelectableFlags flags = 0, int width = 0, int height = 0); \r\n"
"  \r\n"
" // Widgets: Drag \r\n"
"  \r\n"
" /// CTRL+Click on any drag box to turn them into an input box. Manually input values aren't clamped and can go off-bounds. \r\n"
" import static float DragFloat(String label, float value, float min_value = 0, float max_value = 0, float speed = 0, String format = 0); \r\n"
"  \r\n"
" /// CTRL+Click on any drag box to turn them into an input box. Manually input values aren't clamped and can go off-bounds. \r\n"
" import static int DragInt(String label, int value, int min_value = 0, int max_value = 0, float speed = 0, String format = 0); \r\n"
"  \r\n"
" // Widgets: Slider \r\n"
"  \r\n"
" /// CTRL+Click on any slider to turn them into an input box. Manually input values aren't clamped and can go off-bounds. \r\n"
" import static float SliderFloat(String label, float value, float min_value = 0, float max_value = 0, String format = 0); \r\n"
"  \r\n"
" /// CTRL+Click on any slider to turn them into an input box. Manually input values aren't clamped and can go off-bounds. \r\n"
" import static int SliderInt(String label, int value, int min_value = 0, int max_value = 0, String format = 0); \r\n"
"  \r\n"
" // Widgets: Input with Keyboard \r\n"
"  \r\n"
" import static String InputText(String label, String text_buffer, int buffer_size, ImGuiInputTextFlags flags =0); \r\n"
"  \r\n"
" import static String InputTextMultiline(String label, String text_buffer, int buffer_size, int width=0, int height=0, ImGuiInputTextFlags flags = 0); \r\n"
"  \r\n"
" import static String InputTextWithHint(String label, String hint, String text_buffer, int buffer_size, ImGuiInputTextFlags flags = 0); \r\n"
"  \r\n"
" // Widgets: Combobox commands \r\n"
"  \r\n"
" /// The BeginCombo()/EndCombo() allows to manage your contents and selection state however you want it, by creating e.g. Selectable() items. \r\n"
" import static bool BeginCombo(String label, String preview_value, ImGuiComboFlags flags = 0); \r\n"
"  \r\n"
" /// Only call EndCombo() if BeginCombo() returns true! \r\n"
" import static void EndCombo(); \r\n"
"  \r\n"
" // Widgets: List Boxes \r\n"
"  \r\n"
" /// If the function return true, you can output elements then call EndListBox() afterwards. \r\n"
" import static bool BeginListBox(String label, int items_count, int height_in_items = -1); \r\n"
"  \r\n"
" /// Only call EndListBox() if BeginListBox() returns true! \r\n"
" import static void EndListBox(); \r\n"\
"  \r\n"
" // Tooltips \r\n"
" // - Tooltip are windows following the mouse which do not take focus away. \r\n"
"  \r\n"
" /// begin/append a tooltip window. to create full-featured tooltip (with any kind of items).  \r\n"
" import static void BeginTooltip(); \r\n"
"  \r\n"
" /// always call after a BeginTooltip block \r\n"
" import static void EndTooltip(); \r\n"
"  \r\n"
" /// set a text-only tooltip, typically use with AgsImGui.IsItemHovered(). override any previous call to SetTooltip(). \r\n"
" import static void SetTooltip(String text); \r\n"
"  \r\n"
"  \r\n"
" /// call to mark popup as open (don't call every frame!). popups are closed when user click outside, or if CloseCurrentPopup() is called within a BeginPopup()/EndPopup() block. Popup identifiers are relative to the current ID-stack. \r\n"
" import static void OpenPopup(String str_id); \r\n"
"  \r\n"
" /// return true if the popup is open, and you can start outputting to it. only call EndPopup() if BeginPopup() returns true! \r\n"
" import static bool BeginPopup(String str_id, ImGuiWindowFlags flags = 0);  \r\n"
"  \r\n"
" /// modal dialog (regular window with title bar, block interactions behind the modal window, can't close the modal window by clicking outside) \r\n"
" import static bool BeginPopupModal(String name, bool has_close_button = 0, ImGuiWindowFlags flags = 0); \r\n"
"  \r\n"
" /// only call EndPopup() if BeginPopupXXX() returns true! \r\n"
" import static void EndPopup(); \r\n"
"  \r\n"
" /// return true if the popup is open at the current begin-ed level of the popup stack. \r\n"
" import static bool IsPopupOpen(String str_id); \r\n"
"  \r\n"
" /// close the popup we have begin-ed into. clicking on a MenuItem or Selectable automatically close the current popup. \r\n"
" import static void CloseCurrentPopup(); \r\n"
"  \r\n"
" // Tab Bars, Tabs \r\n"
"  \r\n"
" /// create and append into a TabBar \r\n"
" import static bool BeginTabBar(String str_id, ImGuiTabBarFlags flags = 0); \r\n"
"  \r\n"
" /// only call EndTabBar() if BeginTabBar() returns true! \r\n"
" import static void EndTabBar(); \r\n"
"  \r\n"
" /// create a Tab. Returns true if the Tab is selected. \r\n"
" import static bool BeginTabItem(String label, bool has_close_button = 0, ImGuiTabItemFlags flags = 0); \r\n"
"  \r\n"
" /// only call EndTabItem() if BeginTabItem() returns true! \r\n"
" import static void EndTabItem(); \r\n"
"  \r\n"
" /// notify TabBar or Docking system of a closed tab/window ahead (useful to reduce visual flicker on reorderable tab bars). For tab-bar: call after BeginTabBar() and before Tab submissions. Otherwise call with a window name. \r\n"
" import static void SetTabItemClosed(String tab_or_docked_window_label); \r\n"
"  \r\n"
" /// append to menu-bar of current window (requires ImGuiWindowFlags_MenuBar flag set on parent window). \r\n"
" import static bool BeginMenuBar(); \r\n"
"  \r\n"
" /// Only call EndMenuBar() if BeginMenuBar() returns true! \r\n"
" import static void EndMenuBar(); \r\n"
"  \r\n"
" /// Create and append to a full screen menu-bar. \r\n"
" import static bool BeginMainMenuBar(); \r\n"
"  \r\n"
" /// Only call EndMainMenuBar() if BeginMainMenuBar() returns true! \r\n"
" import static void EndMainMenuBar(); \r\n"
"  \r\n"
" /// Create a sub-menu entry. Only call EndMenu() if this returns true! \r\n"
" import static bool BeginMenu(String label, bool enabled = true); \r\n"
"  \r\n"
" /// Only call EndMenu() if BeginMenu() returns true! \r\n"
" import static void EndMenu(); \r\n"
"  \r\n"
" /// return true when activated. shortcuts are displayed for convenience but not processed by ImGui at the moment! \r\n"
" import static bool MenuItem(String label, String shortcut, bool selected = false, bool enabled = true); \r\n"
"  \r\n"
" // Widgets: Value \r\n"
"  \r\n"
" /// Shortcut to a label followed by a boolean value. \r\n"
" import static void ValueBool(String prefix = 0, bool value); \r\n"
"  \r\n"
" /// Shortcut to a label followed by a int value. \r\n"
" import static void ValueInt(String prefix = 0, int value); \r\n"
"  \r\n"
" /// Shortcut to a label followed by a float value. \r\n"
" import static void ValueFloat(String prefix = 0, float value); \r\n"
" // io stuff \r\n"
" /// Override capture or not capture mouse by ImGui for next frame. Mouse will still be captured by AGS. \r\n"
" import static void DoCaptureMouse(bool want_capture_mouse = true); \r\n"
"  \r\n"
" /// Override capture or not capture keyboard by ImGui for next frame. Keyboard will still be captured by AGS. \r\n"
" import static void DoCaptureKeyboard(bool want_capture_keyboard = true); \r\n"
"  \r\n"
" /// Override capture or not capture keyboard by ImGui for next frame. Keyboard will still be captured by AGS. \r\n"
" import static void DoMouseWheel(ImGuiDir wheel_direction); \r\n"
" }; \r\n"
"  \r\n"
" struct AgsImGuiHelper { \r\n"
"  \r\n"
"  import static void SetClipboarText(String text); \r\n"
"  \r\n"
"  import static String GetClipboarText(); \r\n"
"  \r\n"
"  //import static void SetClipboarImage(int sprite_id); \r\n"
"  \r\n"
"  //import static int GetClipboarImage(); \r\n"
"  \r\n"
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
            (float)((getr32(color))/255.0),
            (float)((getg32(color))/255.0),
            (float)((getb32(color))/255.0),
            (float)((geta32(color))/255.0));
}



// Engine interface

//------------------------------------------------------------------------------

union
{
    float f;
    uint32_t ui32;
} AgsNumber;

uint32_t inline ToAgsFloat(float f) {
    AgsNumber.f = f;
    return AgsNumber.ui32;
}

float inline ToNormalFloat(uint32_t ui32) {
    AgsNumber.ui32 = ui32;
    return AgsNumber.f;
}

int inline ToAgsBool(bool b){
    return b ? 1 : 0;
}

std::string g_ClipboardTextData = "";

static const char* ImGui_ImplClip_GetClipboardText(void*)
{
    g_ClipboardTextData.clear();
	if (clip::has(clip::text_format())) {
		clip::get_text(g_ClipboardTextData);
	}     
    return g_ClipboardTextData.c_str();
}

static void ImGui_ImplClip_SetClipboardText(void*, const char* text)
{
    clip::set_text(text);
}

#define STRINGIFY(s) STRINGIFY_X(s)
#define STRINGIFY_X(s) #s


texture_color32_t software_renderer_screen;
texture_alpha8_t fontAtlas;
ImGuiContext *context = nullptr;

typedef int (*SCAPI_MOUSE_ISBUTTONDOWN) (int button);
SCAPI_MOUSE_ISBUTTONDOWN Mouse_IsButtonDown = NULL;
bool has_new_render = false;
bool has_new_frame = false;
void AgsImGui_NewFrame(){
	if (!screen.initialized) return;

	if (screen.driver == Screen::Driver::eOpenGL) {

	}
	if (screen.driver == Screen::Driver::eDirectx9) {
		ImGui_ImplDX9_NewFrame();
	}
	if (screen.driver == Screen::Driver::eSoftware) {
		ImGui_ImplSoftraster_NewFrame();
	}
    ImGui::NewFrame();
	has_new_frame = true;
}

void AgsImGui_EndFrame(){
    ImGui::EndFrame();
}

void AgsImGui_Render(){
	ImGui::Render();
    has_new_render = true;
}

//int AgsImGui_GetDrawData(){
//    return 0;
//    if(screen.driver == Screen::Driver::eSoftware) {
//        return ImGui_ImplSoftraster_GetSprite();
//    }
//}

const char* AgsImGui_GetVersion(){
    return engine->CreateScriptString(ImGui::GetVersion());
}

int AgsImGui_BeginWindow(const char* name, int has_close_button, int32 flags = 0){
    bool p_open = true;
    bool not_collapsed = ImGui::Begin(name, (has_close_button != 0 ? &p_open : nullptr), flags);

    if(p_open && !not_collapsed) return 4;
    else if(!p_open && not_collapsed) return 3;
    else if(!p_open && !not_collapsed) return 2;
    return 1;
}

void AgsImGui_EndWindow(){
    ImGui::End();
}

void AgsImGui_ShowDemoWindow(){
    ImGui::ShowDemoWindow();
}

void AgsImGui_ShowAboutWindow(){
    ImGui::ShowAboutWindow();
}

void AgsImGui_ShowMetricsWindow(){
    ImGui::ShowMetricsWindow();
}

void AgsImGui_ShowUserGuide(){
    ImGui::ShowUserGuide();
}

void AgsImGui_StyleColorsDark(){
    ImGui::StyleColorsDark();
}

void AgsImGui_StyleColorsClassic(){
    ImGui::StyleColorsClassic();
}

void AgsImGui_StyleColorsLight(){
    ImGui::StyleColorsLight();
}

int AgsImGui_BeginChild(const char* str_id, int width = 0, int height = 0, bool border = false, int32 flags = 0){
    return ToAgsBool(ImGui::BeginChild(str_id,ImVec2((float) width,(float) height), border, flags));
}

void AgsImGui_EndChild(){
    ImGui::EndChild();
}

void AgsImGui_SetNextWindowPos(int position_x, int position_y, int cond, uint32_t pivot_x, uint32_t pivot_y){
    float f_pivot_x = ToNormalFloat(pivot_x);
    float f_pivot_y = ToNormalFloat(pivot_y);

    ImGui::SetNextWindowPos(ImVec2((float) position_x, (float)position_y), cond, ImVec2(f_pivot_x,f_pivot_y));
}

void AgsImGui_SetNextWindowSize(int width, int height, int cond){
    ImGui::SetNextWindowPos(ImVec2((float) width, (float)height), cond);
}

void AgsImGui_SetNextWindowSizeConstraints(int min_width, int min_height, int max_width, int max_height){
    ImGui::SetNextWindowSizeConstraints(ImVec2((float) min_width, (float)min_height),ImVec2((float) max_width, (float)max_height));
}

void AgsImGui_SetNextWindowContentSize(int width, int height){
    ImGui::SetNextWindowContentSize(ImVec2((float) width, (float)height));
}

void AgsImGui_SetNextWindowCollapsed(int collapsed, int cond){
    ImGui::SetNextWindowCollapsed(collapsed != 0, cond);
}

void AgsImGui_SetNextWindowFocus(){
    ImGui::SetNextWindowFocus();
}

void AgsImGui_SetNextWindowBgAlpha(float alpha){
    ImGui::SetNextWindowBgAlpha(alpha);
}

int AgsImGui_IsWindowAppearing(){
    return ToAgsBool(ImGui::IsWindowAppearing());
}

int AgsImGui_IsWindowCollapsed(){
    return ToAgsBool(ImGui::IsWindowCollapsed());
}

int AgsImGui_IsWindowFocused(int flags){
    return ToAgsBool(ImGui::IsWindowFocused(flags));
}

int AgsImGui_IsWindowHovered(int flags){
    return ToAgsBool(ImGui::IsWindowHovered(flags));
}

int AgsImGui_IsItemHovered(int flags){
    return ToAgsBool(ImGui::IsItemHovered(flags));
}

int AgsImGui_IsItemActive(){
    return ToAgsBool(ImGui::IsItemActive());
}

int AgsImGui_IsItemFocused(){
    return ToAgsBool(ImGui::IsItemFocused());
}

int AgsImGui_IsItemVisible(){
    return ToAgsBool(ImGui::IsItemVisible());
}

int AgsImGui_IsItemEdited(){
    return ToAgsBool(ImGui::IsItemEdited());
}

int AgsImGui_IsItemActivated(){
    return ToAgsBool(ImGui::IsItemActivated());
}

int AgsImGui_IsItemDeactivated(){
    return ToAgsBool(ImGui::IsItemDeactivated());
}

int AgsImGui_IsItemDeactivatedAfterEdit(){
    return ToAgsBool(ImGui::IsItemDeactivatedAfterEdit());
}

int AgsImGui_IsItemToggledOpen(){
    return ToAgsBool(ImGui::IsItemToggledOpen());
}

int AgsImGui_IsAnyItemHovered(){
    return ToAgsBool(ImGui::IsAnyItemHovered());
}

int AgsImGui_IsAnyItemActive(){
    return ToAgsBool(ImGui::IsAnyItemActive());
}

int AgsImGui_IsAnyItemFocused(){
    return ToAgsBool(ImGui::IsAnyItemFocused());
}

void AgsImGui_Separator () {
    ImGui::Separator();
}

void AgsImGui_SameLine (uint32_t offset_from_start_x, uint32_t spacing) {
    float f_offset_from_start_x = ToNormalFloat(offset_from_start_x);
    float f_spacing = ToNormalFloat(spacing);
    ImGui::SameLine(f_offset_from_start_x,f_spacing);
}

void AgsImGui_NewLine () {
    ImGui::NewLine();
}

void AgsImGui_Spacing () {
    ImGui::Spacing();
}

void AgsImGui_Dummy (uint32_t width, uint32_t height) {
    float f_width = ToNormalFloat(width);
    float f_height = ToNormalFloat(height);
    ImGui::Dummy(ImVec2(f_width,f_height));
}

void AgsImGui_Indent (uint32_t indent_w) {
    float f_indent_w = ToNormalFloat(indent_w);
    ImGui::Indent(f_indent_w);
}

void AgsImGui_Unindent (uint32_t indent_w) {
    float f_indent_w = ToNormalFloat(indent_w);
    ImGui::Unindent(f_indent_w);
}

void AgsImGui_PushID(const char* str_id) {
    ImGui::PushID(str_id);
}

void AgsImGui_PopID() {
    ImGui::PopID();
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

int AgsImGui_Button(const char* label, int width, int height){
    return ToAgsBool(ImGui::Button(label, ImVec2((float) width, (float) height)));
}

int AgsImGui_SmallButton(const char* label){
    return ToAgsBool(ImGui::SmallButton(label));
}

void AgsImGui_Image(int sprite_id){
    int sprite_width = engine->GetSpriteWidth(sprite_id);
    int sprite_height = engine->GetSpriteHeight(sprite_id);
    if(screen.driver == Screen::Driver::eSoftware) {
        ImGui::Image(ImGui_ImplSoftraster_SpriteIDToTexture(sprite_id),
                     ImVec2((float) sprite_width, (float) sprite_height));
    }
    if(screen.driver == Screen::Driver::eDirectx9) {
        ImGui::Image(ImGui_ImplDX9_SpriteIDToTexture(sprite_id),
                     ImVec2((float) sprite_width, (float) sprite_height));
    }
}

int AgsImGui_ImageButton(int sprite_id){
    int sprite_width = engine->GetSpriteWidth(sprite_id);
    int sprite_height = engine->GetSpriteHeight(sprite_id);
    if(screen.driver == Screen::Driver::eSoftware) {
        return ToAgsBool(ImGui::ImageButton(ImGui_ImplSoftraster_SpriteIDToTexture(sprite_id),
                                  ImVec2((float) sprite_width, (float) sprite_height)));
    }
    if(screen.driver == Screen::Driver::eDirectx9) {
        return ToAgsBool(ImGui::ImageButton(ImGui_ImplDX9_SpriteIDToTexture(sprite_id),
                     ImVec2((float) sprite_width, (float) sprite_height)));
    }
    return 0;
}

int AgsImGui_ArrowButton(const char* str_id, int32 dir){
    return ToAgsBool(ImGui::ArrowButton(str_id, dir));
}

int AgsImGui_Checkbox(const char* label, int v){
    bool value = v != 0;
    return ToAgsBool(ImGui::Checkbox(label, &value));
}

int AgsImGui_RadioButton(const char* label, bool active){
    return ToAgsBool(ImGui::RadioButton(label, active));
}

void AgsImGui_Bullet(){
    ImGui::Bullet();
}

int AgsImGui_Selectable(const char* label, int selected, int flags, int width, int height){
    return ToAgsBool(ImGui::Selectable(label, selected != 0, flags, ImVec2((float) width, (float) height)));
}

int AgsImGui_BeginCombo(const char* name, const char* preview_value, int32 flags = 0){
    return ToAgsBool(ImGui::BeginCombo(name, preview_value, flags));
}

void AgsImGui_EndCombo(){
    ImGui::EndCombo();
}

uint32_t AgsImGui_DragFloat(const char* label, uint32_t value, uint32_t v_min, uint32_t v_max, uint32_t speed, const char* format){
    std::string format_string =  "%.3f";
    std::string empty_string =  "";
    float f_value = ToNormalFloat(value);
    float f_speed = ToNormalFloat(speed);
    float f_v_min = ToNormalFloat(v_min);
    float f_v_max = ToNormalFloat(v_max);
    if(f_speed == 0.0) f_speed = 1.0;
    if(format == nullptr) format = format_string.c_str();
    if(label == nullptr) label = empty_string.c_str();

    ImGui::DragFloat(label, &f_value, f_speed, f_v_min, f_v_max, format);
    return ToAgsFloat(f_value);
}

int AgsImgui_DragInt(const char* label, int value, int v_min, int v_max, uint32_t speed, const char* format){
    std::string format_string =  "%d";
    std::string empty_string =  "";
    float f_speed = ToNormalFloat(speed);
    if(f_speed == 0.0) f_speed = 1.0;
    if(format == nullptr) format = format_string.c_str();
    if(label == nullptr) label = empty_string.c_str();

    int ret_value = value;
    ImGui::DragInt(label, &ret_value, f_speed, v_min, v_max, format);
    return ret_value;
}

uint32_t AgsImGui_SliderFloat(const char* label, uint32_t value, uint32_t v_min, uint32_t v_max, const char* format){
    std::string format_string =  "%.3f";
    std::string empty_string =  "";
    float f_value = ToNormalFloat(value);
    float f_v_min = ToNormalFloat(v_min);
    float f_v_max = ToNormalFloat(v_max);
    if(format == nullptr) format = format_string.c_str();
    if(label == nullptr) label = empty_string.c_str();

    ImGui::SliderFloat(label, &f_value, f_v_min, f_v_max, format);
    return ToAgsFloat(f_value);
}

int AgsImgui_SliderInt(const char* label, int value, int v_min, int v_max, const char* format){
    std::string format_string =  "%d";
    std::string empty_string =  "";
    if(format == nullptr) format = format_string.c_str();
    if(label == nullptr) label = empty_string.c_str();

    int ret_value = value;
    ImGui::SliderInt(label, &ret_value, v_min, v_max, format);
    return ret_value;
}

const char* AgsImgui_InputText(const char* label, char* buf, int buf_size, int flags) {
    if(buf_size <= 1) engine->AbortGame("Buffer size can't be smaller than 2");
    if(strlen(buf) > buf_size) engine->AbortGame("Buffer size smaller than buffer string on Input Text");

    char * resized_buffer = new char [buf_size];
    std::strcpy(resized_buffer, buf);
    bool changed =  ImGui::InputText(label,resized_buffer, buf_size,flags);
    if(changed) {
        return  engine->CreateScriptString(resized_buffer);
    }
    delete[] resized_buffer;
    return nullptr;
}

const char* AgsImgui_InputTextMultiline(const char* label, const char* buf, int buf_size, int width, int height, int flags) {
    if(buf_size <= 1) engine->AbortGame("Buffer size can't be smaller than 2");
    if(strlen(buf) > buf_size) engine->AbortGame("Buffer size smaller than buffer string on Input Text");

    char * resized_buffer = new char [buf_size];
    std::strcpy(resized_buffer, buf);
    bool changed =  ImGui::InputTextMultiline(label,resized_buffer, buf_size, ImVec2((float) width, (float) height), flags);
    if(changed) {
        return  engine->CreateScriptString(resized_buffer);
    }
    delete[] resized_buffer;
    return nullptr;
}

const char* AgsImgui_InputTextWithHint(const char* label, const char* hint, const char* buf, int buf_size, int flags) {
    if(buf_size <= 1) engine->AbortGame("Buffer size can't be smaller than 2");
    if(strlen(buf) > buf_size) engine->AbortGame("Buffer size smaller than buffer string on Input Text");

    char * resized_buffer = new char [buf_size];
    std::strcpy(resized_buffer, buf);
    bool changed =  ImGui::InputTextWithHint(label, hint, resized_buffer, buf_size,flags);
    if(changed) {
        return  engine->CreateScriptString(resized_buffer);
    }
    delete[] resized_buffer;
    return nullptr;
}

int AgsImGui_BeginListBox(const char* name, int items_count, int height_in_items = -1){
    return ToAgsBool(ImGui::ListBoxHeader(name,items_count,height_in_items));
}

void AgsImGui_EndListBox(){
    ImGui::ListBoxFooter();
}

void AgsImGui_BeginTooltip(){
    ImGui::BeginTooltip();
}

void AgsImGui_EndTooltip(){
    ImGui::EndTooltip();
}

void AgsImGui_SetTooltip(const char * text){
    ImGui::SetTooltip(text);
}

void AgsImGui_OpenPopup(const char* str_id) {
    ImGui::OpenPopup(str_id);
}

int AgsImGui_BeginPopup(const char* str_id, int flags) {
    return ToAgsBool(ImGui::BeginPopup(str_id, flags));
}

int AgsImGui_BeginPopupModal(const char* name, bool has_close_button, int flags) {
    bool p_open = true;
    return ToAgsBool(ImGui::BeginPopupModal(name,(has_close_button != 0 ? &p_open : nullptr),flags));
}

void AgsImGui_EndPopup() {
    ImGui::EndPopup();
}

int AgsImGui_IsPopupOpen(const char* str_id) {
    return ToAgsBool(ImGui::IsPopupOpen(str_id));
}

void AgsImGui_CloseCurrentPopup() {
    ImGui::CloseCurrentPopup();
}

int AgsImGui_BeginTabBar(const char * str_id, int flags){
    return ToAgsBool(ImGui::BeginTabBar(str_id, flags));
}

void AgsImGui_EndTabBar(){
    ImGui::EndTabBar();
}

int AgsImGui_BeginTabItem(const char * label,  bool has_close_button, int flags){
    bool p_open = false;
    return ToAgsBool(ImGui::BeginTabItem(label, (has_close_button != 0 ? &p_open : nullptr), flags));
}

void AgsImGui_EndTabItem(){
    ImGui::EndTabItem();
}

void AgsImGui_SetTabItemClosed(const char * tab_or_docked_window_label){
    ImGui::SetTabItemClosed(tab_or_docked_window_label);
}


int AgsImGui_BeginMenuBar(){
    return ToAgsBool(ImGui::BeginMenuBar());
}

void AgsImGui_EndMenuBar(){
    ImGui::EndMenuBar();
}


int AgsImGui_BeginMainMenuBar(){
    return ToAgsBool(ImGui::BeginMainMenuBar());
}

void AgsImGui_EndMainMenuBar(){
    ImGui::EndMainMenuBar();
}

int AgsImGui_BeginMenu(const char* name, int enabled){
    return ToAgsBool(ImGui::BeginMenu(name,enabled != 0));
}

void AgsImGui_EndMenu(){
    ImGui::EndMenu();
}

int AgsImGui_MenuItem(const char* label, const char* shortcut, bool selected = false, bool enabled = true){
    return  ToAgsBool(ImGui::MenuItem(label, shortcut, &selected, enabled));
}

void AgsImGui_DoCaptureMouse(int want_capture_mouse){
    ImGui::CaptureMouseFromApp(want_capture_mouse != 0);
}

void AgsImGui_DoCaptureKeyboard(int want_capture_keyboard){
    ImGui::CaptureKeyboardFromApp(want_capture_keyboard != 0);
}


void AgsImGui_ValueBool(const char* prefix, int value){
    if(prefix == nullptr)  {
        std::string empty = "";
        prefix = empty.c_str();
    }
    ImGui::Value(prefix,value != 0);
}

void AgsImGui_ValueInt(const char* prefix, int value){
    if(prefix == nullptr)  {
        std::string empty = "";
        prefix = empty.c_str();
    }
    ImGui::Value(prefix,value);
}

void AgsImGui_ValueFloat(const char* prefix, uint32_t value){
    if(prefix == nullptr) {
        std::string empty = "";
        prefix = empty.c_str();
    }
    ImGui::Value(prefix,ToNormalFloat(value));
}

void AgsImGui_DoMouseWheel(int wheel_dir) {
	ImGuiIO& io = ImGui::GetIO();

	if (wheel_dir == 0) io.MouseWheelH -= 1;
	else if (wheel_dir == 1) io.MouseWheelH += 1;
	else if (wheel_dir == 2) io.MouseWheel -= 1;
	else if (wheel_dir == 3) io.MouseWheel += 1;
}


void AgsImGuiHelper_SetClipboarText(const char* text) {
	clip::set_text(text);
}

const char* AgsImGuiHelper_GetClipboarText() {
	if (!clip::has(clip::text_format())) {
		return engine->CreateScriptString("");
	}

	g_ClipboardTextData.clear();
	clip::get_text(g_ClipboardTextData);
	return engine->CreateScriptString(g_ClipboardTextData.c_str());
}

void AgsImGuiHelper_SetClipboarImage(int sprite_id) {
	BITMAP* engineSprite = engine->GetSpriteGraphic(sprite_id);
	int sprite_width = engine->GetSpriteWidth(sprite_id);
	int sprite_height = engine->GetSpriteHeight(sprite_id);

	unsigned char** charbuffer = engine->GetRawBitmapSurface(engineSprite);
	uint32_t** longbuffer = (uint32_t * *)charbuffer;
	char* single_char_buffer = (char*)charbuffer;
	
	clip::image_spec spec;
	spec.width = sprite_width;
	spec.height = sprite_height;
	spec.bits_per_pixel = 32;
	spec.bytes_per_row = spec.width * 4;
	spec.red_mask = 0x00ff0000;
	spec.green_mask = 0xff00;
	spec.blue_mask = 0xff;
	spec.alpha_mask = 0xff000000;
	spec.red_shift = 16;
	spec.green_shift = 8;
	spec.blue_shift = 0;
	spec.alpha_shift = 24;
	clip::image img(single_char_buffer, spec);
	clip::set_image(img);
	engine->ReleaseBitmapSurface(engineSprite);
}

int AgsImGuiHelper_GetClipboarImage() {
	if (!clip::has(clip::image_format())) {
		// no image on clipboard
		return 0;
	}

	clip::image img;
	if (!clip::get_image(img)) {
		// Error getting image from clipboard
		return 0;
	}

	clip::image_spec spec = img.spec();

	int sprite_width = spec.width;
	int sprite_height = spec.height;
	int color_depth = spec.bits_per_pixel;

	if (color_depth != 32) {
		return 0;
	}

	int sprite_id = engine->CreateDynamicSprite(color_depth, sprite_width, sprite_height);
	BITMAP* engineSprite = engine->GetSpriteGraphic(sprite_id);

	unsigned char** charbuffer = engine->GetRawBitmapSurface(engineSprite);
	uint32_t** longbuffer = (uint32_t * *)charbuffer;

	for (int ix = 0; ix < sprite_width; ix++) {
		for (int iy = 0; iy < sprite_height; iy++) {
			
			longbuffer[iy][ix] = ((uint32_t * *)(img.data()))[iy][ix];
		}
	}

	engine->ReleaseBitmapSurface(engineSprite);
	return sprite_id;
}


	void AGS_EngineStartup(IAGSEngine *lpEngine)
	{
		engine = lpEngine;

		// Make sure it's got the version with the features we need
		if (engine->version < MIN_ENGINE_VERSION)
			engine->AbortGame("Plugin needs engine version " STRINGIFY(MIN_ENGINE_VERSION) " or newer.");

		//register functions
        if(screen.driver == Screen::Driver::eOpenGL) {

        }
        if(screen.driver == Screen::Driver::eDirectx9) {
			
        }
        if(screen.driver == Screen::Driver::eSoftware) {
            ImGui_ImplSoftraster_InitializeEngine(engine);
        }

        context = ImGui::CreateContext();

        if(screen.driver == Screen::Driver::eOpenGL) {

        }
        if(screen.driver == Screen::Driver::eDirectx9) {

        }
        if(screen.driver == Screen::Driver::eSoftware) {
            ImGui_ImplSoftraster_Init(&software_renderer_screen);
        }

        ImGuiStyle& style = ImGui::GetStyle();
        style.AntiAliasedLines = false;
        style.AntiAliasedFill = false;
        style.WindowRounding = 0.0f;
        style.Alpha = 1.0f;

        ImGui::StyleColorsDark();

        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight | ImFontAtlasFlags_NoMouseCursors;

		if (screen.driver == Screen::Driver::eSoftware) {
			uint8_t* pixels;
			int width, height;
			io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
			fontAtlas.init(width, height, (alpha8_t*)pixels);
			io.Fonts->TexID = &fontAtlas;
		}

        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;       // We can honor GetMouseCursor() values (optional)
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        io.KeyMap[ImGuiKey_Tab] = eAGSKeyCodeTab;
        io.KeyMap[ImGuiKey_LeftArrow] = eAGSKeyCodeLeftArrow;
        io.KeyMap[ImGuiKey_RightArrow] = eAGSKeyCodeRightArrow;
        io.KeyMap[ImGuiKey_UpArrow] = eAGSKeyCodeUpArrow;
        io.KeyMap[ImGuiKey_DownArrow] = eAGSKeyCodeDownArrow;
        io.KeyMap[ImGuiKey_PageUp] = eAGSKeyCodePageUp;
        io.KeyMap[ImGuiKey_PageDown] = eAGSKeyCodePageDown;
        io.KeyMap[ImGuiKey_Home] = eAGSKeyCodeHome;
        io.KeyMap[ImGuiKey_End] = eAGSKeyCodeEnd;
        io.KeyMap[ImGuiKey_Insert] = eAGSKeyCodeInsert;
        io.KeyMap[ImGuiKey_Delete] = eAGSKeyCodeDelete;
        io.KeyMap[ImGuiKey_Backspace] = eAGSKeyCodeBackspace;
        io.KeyMap[ImGuiKey_Space] = eAGSKeyCodeSpace;
        io.KeyMap[ImGuiKey_Enter] = eAGSKeyCodeReturn;
        io.KeyMap[ImGuiKey_Escape] = eAGSKeyCodeEscape;
        io.KeyMap[ImGuiKey_KeyPadEnter] = eAGSKeyCodeReturn;
        io.KeyMap[ImGuiKey_A] = eAGSKeyCodeA;
        io.KeyMap[ImGuiKey_C] = eAGSKeyCodeC;
        io.KeyMap[ImGuiKey_V] = eAGSKeyCodeV;
        io.KeyMap[ImGuiKey_X] = eAGSKeyCodeX;
        io.KeyMap[ImGuiKey_Y] = eAGSKeyCodeY;
        io.KeyMap[ImGuiKey_Z] = eAGSKeyCodeZ;
        io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);

        io.SetClipboardTextFn = ImGui_ImplClip_SetClipboardText;
        io.GetClipboardTextFn = ImGui_ImplClip_GetClipboardText;
        io.ClipboardUserData = NULL;

        Mouse_IsButtonDown = (SCAPI_MOUSE_ISBUTTONDOWN) engine->GetScriptFunctionAddress("Mouse::IsButtonDown^1");

        engine->RegisterScriptFunction("AgsImGui::NewFrame^0", (void*)AgsImGui_NewFrame);
        engine->RegisterScriptFunction("AgsImGui::EndFrame^0", (void*)AgsImGui_EndFrame);
        engine->RegisterScriptFunction("AgsImGui::Render^0", (void*)AgsImGui_Render);
        //engine->RegisterScriptFunction("AgsImGui::GetDrawData^0", (void*)AgsImGui_GetDrawData);
        engine->RegisterScriptFunction("AgsImGui::GetVersion^0", (void*)AgsImGui_GetVersion);
        engine->RegisterScriptFunction("AgsImGui::ShowDemoWindow^0", (void*)AgsImGui_ShowDemoWindow);
        engine->RegisterScriptFunction("AgsImGui::ShowAboutWindow^0", (void*)AgsImGui_ShowAboutWindow);
        engine->RegisterScriptFunction("AgsImGui::ShowMetricsWindow^0", (void*)AgsImGui_ShowMetricsWindow);
        engine->RegisterScriptFunction("AgsImGui::ShowUserGuide^0", (void*)AgsImGui_ShowUserGuide);
        engine->RegisterScriptFunction("AgsImGui::StyleColorsDark^0", (void*)AgsImGui_StyleColorsDark);
        engine->RegisterScriptFunction("AgsImGui::StyleColorsClassic^0", (void*)AgsImGui_StyleColorsClassic);
        engine->RegisterScriptFunction("AgsImGui::StyleColorsLight^0", (void*)AgsImGui_StyleColorsLight);
        engine->RegisterScriptFunction("AgsImGui::BeginWindow^3", (void*)AgsImGui_BeginWindow);
        engine->RegisterScriptFunction("AgsImGui::EndWindow^0", (void*)AgsImGui_EndWindow);
        engine->RegisterScriptFunction("AgsImGui::BeginChild^5", (void*)AgsImGui_BeginChild);
        engine->RegisterScriptFunction("AgsImGui::EndChild^0", (void*)AgsImGui_EndChild);
        engine->RegisterScriptFunction("AgsImGui::SetNextWindowPos^5", (void*)AgsImGui_SetNextWindowPos);
        engine->RegisterScriptFunction("AgsImGui::SetNextWindowSize^3", (void*)AgsImGui_SetNextWindowSize);
        engine->RegisterScriptFunction("AgsImGui::SetNextWindowSizeConstraints^4", (void*)AgsImGui_SetNextWindowSizeConstraints);
        engine->RegisterScriptFunction("AgsImGui::SetNextWindowContentSize^2", (void*)AgsImGui_SetNextWindowContentSize);
        engine->RegisterScriptFunction("AgsImGui::SetNextWindowCollapsed^2", (void*)AgsImGui_SetNextWindowCollapsed);
        engine->RegisterScriptFunction("AgsImGui::SetNextWindowFocus^0", (void*)AgsImGui_SetNextWindowFocus);
        engine->RegisterScriptFunction("AgsImGui::SetNextWindowBgAlpha^1", (void*)AgsImGui_SetNextWindowBgAlpha);
        engine->RegisterScriptFunction("AgsImGui::IsWindowAppearing^0", (void*)AgsImGui_IsWindowAppearing);
        engine->RegisterScriptFunction("AgsImGui::IsWindowCollapsed^0", (void*)AgsImGui_IsWindowCollapsed);
        engine->RegisterScriptFunction("AgsImGui::IsWindowFocused^1", (void*)AgsImGui_IsWindowFocused);
        engine->RegisterScriptFunction("AgsImGui::IsWindowHovered^1", (void*)AgsImGui_IsWindowHovered);
        engine->RegisterScriptFunction("AgsImGui::IsItemHovered^1", (void*)AgsImGui_IsItemHovered);
        engine->RegisterScriptFunction("AgsImGui::IsItemActive^0", (void*)AgsImGui_IsItemActive);
        engine->RegisterScriptFunction("AgsImGui::IsItemFocused^0", (void*)AgsImGui_IsItemFocused);
        engine->RegisterScriptFunction("AgsImGui::IsItemVisible^0", (void*)AgsImGui_IsItemVisible);
        engine->RegisterScriptFunction("AgsImGui::IsItemEdited^0", (void*)AgsImGui_IsItemEdited);
        engine->RegisterScriptFunction("AgsImGui::IsItemActivated^0", (void*)AgsImGui_IsItemActivated);
        engine->RegisterScriptFunction("AgsImGui::IsItemDeactivated^0", (void*)AgsImGui_IsItemDeactivated);
        engine->RegisterScriptFunction("AgsImGui::IsItemDeactivatedAfterEdit^0", (void*)AgsImGui_IsItemDeactivatedAfterEdit);
        engine->RegisterScriptFunction("AgsImGui::IsItemToggledOpen^0", (void*)AgsImGui_IsItemToggledOpen);
        engine->RegisterScriptFunction("AgsImGui::IsAnyItemHovered^0", (void*)AgsImGui_IsAnyItemHovered);
        engine->RegisterScriptFunction("AgsImGui::IsAnyItemActive^0", (void*)AgsImGui_IsAnyItemActive);
        engine->RegisterScriptFunction("AgsImGui::IsAnyItemFocused^0", (void*)AgsImGui_IsAnyItemFocused);
        engine->RegisterScriptFunction("AgsImGui::Separator^0", (void*)AgsImGui_Separator);
        engine->RegisterScriptFunction("AgsImGui::SameLine^2", (void*)AgsImGui_SameLine);
        engine->RegisterScriptFunction("AgsImGui::NewLine^0", (void*)AgsImGui_NewLine);
        engine->RegisterScriptFunction("AgsImGui::Spacing^0", (void*)AgsImGui_Spacing);
        engine->RegisterScriptFunction("AgsImGui::Dummy^2", (void*)AgsImGui_Dummy);
        engine->RegisterScriptFunction("AgsImGui::Indent^1", (void*)AgsImGui_Indent);
        engine->RegisterScriptFunction("AgsImGui::Unindent^1", (void*)AgsImGui_Unindent);
        engine->RegisterScriptFunction("AgsImGui::PushID^1", (void*)AgsImGui_PushID);
        engine->RegisterScriptFunction("AgsImGui::PopID^0", (void*)AgsImGui_PopID);
        engine->RegisterScriptFunction("AgsImGui::Text^1", (void*)AgsImGui_Text);
        engine->RegisterScriptFunction("AgsImGui::TextColored^2", (void*)AgsImGui_TextColored);
        engine->RegisterScriptFunction("AgsImGui::TextDisabled^1", (void*)AgsImGui_TextDisabled);
        engine->RegisterScriptFunction("AgsImGui::TextWrapped^1", (void*)AgsImGui_TextWrapped);
        engine->RegisterScriptFunction("AgsImGui::LabelText^2", (void*)AgsImGui_LabelText);
        engine->RegisterScriptFunction("AgsImGui::BulletText^1", (void*)AgsImGui_BulletText);
        engine->RegisterScriptFunction("AgsImGui::Button^3", (void*)AgsImGui_Button);
        engine->RegisterScriptFunction("AgsImGui::SmallButton^1", (void*)AgsImGui_SmallButton);
        engine->RegisterScriptFunction("AgsImGui::Image^1", (void*)AgsImGui_Image);
        engine->RegisterScriptFunction("AgsImGui::ImageButton^1", (void*)AgsImGui_ImageButton);
        engine->RegisterScriptFunction("AgsImGui::ArrowButton^2", (void*)AgsImGui_ArrowButton);
        engine->RegisterScriptFunction("AgsImGui::Checkbox^2", (void*)AgsImGui_Checkbox);
        engine->RegisterScriptFunction("AgsImGui::RadioButton^2", (void*)AgsImGui_RadioButton);
        engine->RegisterScriptFunction("AgsImGui::Bullet^0", (void*)AgsImGui_Bullet);
        engine->RegisterScriptFunction("AgsImGui::Selectable^5", (void*)AgsImGui_Selectable);
        engine->RegisterScriptFunction("AgsImGui::DragFloat^6", (void*)AgsImGui_DragFloat);
        engine->RegisterScriptFunction("AgsImGui::DragInt^6", (void*)AgsImgui_DragInt);
        engine->RegisterScriptFunction("AgsImGui::SliderFloat^5", (void*)AgsImGui_SliderFloat);
        engine->RegisterScriptFunction("AgsImGui::SliderInt^5", (void*)AgsImgui_SliderInt);
        engine->RegisterScriptFunction("AgsImGui::InputText^4", (void*)AgsImgui_InputText);
        engine->RegisterScriptFunction("AgsImGui::InputTextMultiline^6", (void*)AgsImgui_InputTextMultiline);
        engine->RegisterScriptFunction("AgsImGui::InputTextWithHint^5", (void*)AgsImgui_InputTextWithHint);
        engine->RegisterScriptFunction("AgsImGui::BeginCombo^3", (void*)AgsImGui_BeginCombo);
        engine->RegisterScriptFunction("AgsImGui::EndCombo^0", (void*)AgsImGui_EndCombo);
        engine->RegisterScriptFunction("AgsImGui::BeginListBox^3", (void*)AgsImGui_BeginListBox);
        engine->RegisterScriptFunction("AgsImGui::EndListBox^0", (void*)AgsImGui_EndListBox);
        engine->RegisterScriptFunction("AgsImGui::BeginTooltip^0", (void*)AgsImGui_BeginTooltip);
        engine->RegisterScriptFunction("AgsImGui::EndTooltip^0", (void*)AgsImGui_EndTooltip);
        engine->RegisterScriptFunction("AgsImGui::SetTooltip^1", (void*)AgsImGui_SetTooltip);
        engine->RegisterScriptFunction("AgsImGui::OpenPopup^1", (void*)AgsImGui_OpenPopup);
        engine->RegisterScriptFunction("AgsImGui::BeginPopup^2", (void*)AgsImGui_BeginPopup);
        engine->RegisterScriptFunction("AgsImGui::BeginPopupModal^3", (void*)AgsImGui_BeginPopupModal);
        engine->RegisterScriptFunction("AgsImGui::EndPopup^0", (void*)AgsImGui_EndPopup);
        engine->RegisterScriptFunction("AgsImGui::IsPopupOpen^1", (void*)AgsImGui_IsPopupOpen);
        engine->RegisterScriptFunction("AgsImGui::CloseCurrentPopup^0", (void*)AgsImGui_CloseCurrentPopup);
        engine->RegisterScriptFunction("AgsImGui::BeginTabBar^2", (void*)AgsImGui_BeginTabBar);
        engine->RegisterScriptFunction("AgsImGui::EndTabBar^0", (void*)AgsImGui_EndTabBar);
        engine->RegisterScriptFunction("AgsImGui::BeginTabItem^3", (void*)AgsImGui_BeginTabItem);
        engine->RegisterScriptFunction("AgsImGui::EndTabItem^0", (void*)AgsImGui_EndTabItem);
        engine->RegisterScriptFunction("AgsImGui::SetTabItemClosed^1", (void*)AgsImGui_SetTabItemClosed);
        engine->RegisterScriptFunction("AgsImGui::BeginMenuBar^0", (void*)AgsImGui_BeginMenuBar);
        engine->RegisterScriptFunction("AgsImGui::EndMenuBar^0", (void*)AgsImGui_EndMenuBar);
        engine->RegisterScriptFunction("AgsImGui::BeginMainMenuBar^0", (void*)AgsImGui_BeginMainMenuBar);
        engine->RegisterScriptFunction("AgsImGui::EndMainMenuBar^0", (void*)AgsImGui_EndMainMenuBar);
        engine->RegisterScriptFunction("AgsImGui::BeginMenu^2", (void*)AgsImGui_BeginMenu);
        engine->RegisterScriptFunction("AgsImGui::EndMenu^0", (void*)AgsImGui_EndMenu);
        engine->RegisterScriptFunction("AgsImGui::MenuItem^4", (void*)AgsImGui_MenuItem);
        engine->RegisterScriptFunction("AgsImGui::DoCaptureMouse^1", (void*)AgsImGui_DoCaptureMouse);
        engine->RegisterScriptFunction("AgsImGui::DoCaptureKeyboard^1", (void*)AgsImGui_DoCaptureKeyboard);
        engine->RegisterScriptFunction("AgsImGui::ValueBool^2", (void*)AgsImGui_ValueBool);
        engine->RegisterScriptFunction("AgsImGui::ValueInt^2", (void*)AgsImGui_ValueInt);
        engine->RegisterScriptFunction("AgsImGui::ValueFloat^2", (void*)AgsImGui_ValueFloat);
		engine->RegisterScriptFunction("AgsImGui::DoMouseWheel^1", (void*)AgsImGui_DoMouseWheel);

		engine->RegisterScriptFunction("AgsImGuiHelper::SetClipboarText^1", (void*)AgsImGuiHelper_SetClipboarText);
		engine->RegisterScriptFunction("AgsImGuiHelper::GetClipboarText^0", (void*)AgsImGuiHelper_GetClipboarText);
		//engine->RegisterScriptFunction("AgsImGuiHelper::SetClipboarImage^1", (void*)AgsImGuiHelper_SetClipboarImage);
		//engine->RegisterScriptFunction("AgsImGuiHelper::GetClipboarImage^0", (void*)AgsImGuiHelper_GetClipboarImage);

        engine->RequestEventHook(AGSE_PRESCREENDRAW);
        engine->RequestEventHook(AGSE_KEYPRESS);
		engine->RequestEventHook(AGSE_POSTSCREENDRAW);
		engine->RequestEventHook(AGSE_MOUSECLICK);
	}

	//------------------------------------------------------------------------------

	void AGS_EngineShutdown()
	{
		// Called by the game engine just before it exits.
		// This gives you a chance to free any memory and do any cleanup
		// that you need to do before the engine shuts down.
		if (screen.driver == Screen::Driver::eOpenGL) {

		}
		if (screen.driver == Screen::Driver::eDirectx9) {
			ImGui_ImplDX9_Shutdown();
		}
		if (screen.driver == Screen::Driver::eSoftware) {
			ImGui_ImplSoftraster_Shutdown();
		}
	}

	//------------------------------------------------------------------------------

enum MouseButton {
    eMouseLeft = 1,
    eMouseRight = 2,
    eMouseMiddle = 3,
    eMouseLeftInv = 5,
    eMouseRightInv = 6,
    eMouseMiddleInv = 7,
    eMouseWheelNorth = 8,
    eMouseWheelSouth = 9
};
    std::vector<int> pressed_keys;
    int32 ags_mouse_x = 0;
    int32 ags_mouse_y = 0;
    bool do_only_once = false;
    int unstuck_counter = 0;
    int AGS_EngineOnEvent(int event, int data)                    //*** optional ***
    {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        

        if(event==AGSE_PRESCREENDRAW){
			ImGuiIO& io = ImGui::GetIO();
			//initialize debug
			if (!screen.initialized) {
				engine->GetScreenDimensions(&screen.width, &screen.height, &screen.colorDepth);
				printf("\nagsimgui 0.1.0\n");

				if (screen.driver == Screen::Driver::eOpenGL) {

					screen.initialized = true;
				}
				if (screen.driver == Screen::Driver::eDirectx9) {
					if ((IDirect3DDevice9*)data != nullptr) {
						io.DisplaySize.x = (float)screen.width;
						io.DisplaySize.y = (float)screen.height;
						ImGui_ImplDX9_Init((IDirect3DDevice9*)data);
						ImGui_ImplDX9_InvalidateDeviceObjects();
						ImGui_ImplDX9_CreateDeviceObjects();
						screen.initialized = true;
					}
				}
				if (screen.driver == Screen::Driver::eSoftware) {
					ImGui_ImplSoftraster_InitializeScreenAgs(screen.width, screen.height, screen.colorDepth);
					software_renderer_screen.init(screen.width, screen.height);
					screen.initialized = true;

				}
			}

            if(!pressed_keys.empty()) {
                unstuck_counter++;

                if(unstuck_counter>5) {
                    unstuck_counter = 0;
                    while (!pressed_keys.empty()) {
                        int key_pressed = pressed_keys.back();
                        io.KeysDown[key_pressed] = false;

                        if( key_pressed == eAGSKeyCodeCtrlC ){
                            io.KeysDown[eAGSKeyCodeC] = false;
                            io.KeyCtrl = false;
                        } else if( key_pressed == eAGSKeyCodeCtrlX ){
                            io.KeysDown[eAGSKeyCodeX] = false;
                            io.KeyCtrl = false;
                        } else if( key_pressed == eAGSKeyCodeCtrlV ){
                            io.KeysDown[eAGSKeyCodeV] = false;
                            io.KeyCtrl = false;
                        } else if( key_pressed == eAGSKeyCodeCtrlZ ){
                            io.KeysDown[eAGSKeyCodeZ] = false;
                            io.KeyCtrl = false;
                        } else if( key_pressed == eAGSKeyCodeCtrlA ){
                            io.KeysDown[eAGSKeyCodeA] = false;
                            io.KeyCtrl = false;
                        } else if( key_pressed == eAGSKeyCodeCtrlY ){
                            io.KeysDown[eAGSKeyCodeY] = false;
                            io.KeyCtrl = false;
                        }
                        pressed_keys.pop_back();

                        // probably is better to keep traversing this and remove only things that have the button lift off, but this is enough for now.
                    }
                }
            }
            else {
                engine->GetMousePosition(&ags_mouse_x, &ags_mouse_y);

                io.MousePos = ImVec2((float) ags_mouse_x, (float) ags_mouse_y);
            }

			io.MouseDown[ImGuiMouseButton_Left] = Mouse_IsButtonDown(eMouseLeft) != 0;
            io.MouseDown[ImGuiMouseButton_Right] = Mouse_IsButtonDown(eMouseRight) != 0;
            io.MouseDown[ImGuiMouseButton_Middle] = Mouse_IsButtonDown(eMouseMiddle) != 0;

        }

        if(event==AGSE_KEYPRESS){
			ImGuiIO& io = ImGui::GetIO();

			if( data == eAGSKeyCodeCtrlC ){
                io.KeysDown[eAGSKeyCodeC] = true;
                io.KeyCtrl = true;
			} else if( data == eAGSKeyCodeCtrlX ){
                io.KeysDown[eAGSKeyCodeX] = true;
                io.KeyCtrl = true;
            } else if( data == eAGSKeyCodeCtrlV ){
                io.KeysDown[eAGSKeyCodeV] = true;
                io.KeyCtrl = true;
            }  else if( data == eAGSKeyCodeCtrlZ ){
                io.KeysDown[eAGSKeyCodeZ] = true;
                io.KeyCtrl = true;
            } else if( data == eAGSKeyCodeCtrlA ){
                io.KeysDown[eAGSKeyCodeA] = true;
                io.KeyCtrl = true;
            } else if( data == eAGSKeyCodeCtrlY ){
                io.KeysDown[eAGSKeyCodeY] = true;
                io.KeyCtrl = true;
            } else {
                io.KeysDown[data] = true;
                io.KeyCtrl = false;
			}

            pressed_keys.push_back(data);

            if(data != 0 &&
               data != eAGSKeyCodeLeftArrow &&
               data != eAGSKeyCodeRightArrow &&
               data != eAGSKeyCodeUpArrow &&
               data != eAGSKeyCodeDownArrow &&
               data != eAGSKeyCodePageUp &&
               data != eAGSKeyCodePageDown && data < 177 ) {
                io.AddInputCharacter(data);
            }
        }

		if (event == AGSE_POSTSCREENDRAW) {
			if (screen.driver == Screen::Driver::eDirectx9) {
				if (has_new_frame && has_new_render) {
					ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
				}
			}
            if (screen.driver == Screen::Driver::eSoftware) {
                if (has_new_frame && has_new_render) {
                    ImGui_ImplSoftraster_RenderDrawData(ImGui::GetDrawData());
                    engine->BlitBitmap(0,0,engine->GetSpriteGraphic(ImGui_ImplSoftraster_GetSprite()),1);
                }
            }
            has_new_frame = false;
            has_new_render = false;
		}

        if(event==AGSE_MOUSECLICK){
			ImGuiIO& io = ImGui::GetIO();

			io.MouseDown[ImGuiMouseButton_Left] |= eMouseLeft == data;
			io.MouseDown[ImGuiMouseButton_Right] |= eMouseRight == data;
			io.MouseDown[ImGuiMouseButton_Middle] |= eMouseMiddle == data;
			
        }

        /*
        switch (event)
        {
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
        default:
            break;
        }
         */

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

    void AGS_EngineInitGfx( char const* driverID, void* data )
    {
        // This allows you to make changes to how the graphics driver starts up.
        #if AGS_PLATFORM_OS_WINDOWS
        if ( strcmp( driverID, "D3D9" ) == 0 )
        {
            D3DPRESENT_PARAMETERS* params = (D3DPRESENT_PARAMETERS*)data;
            if (params->BackBufferFormat != D3DFMT_X8R8G8B8)
            {
                engine->AbortGame( "32bit colour mode required." );
            }
			ImGui_ImplDX9_InitializeEngine(engine);
            screen.backBufferWidth = params->BackBufferWidth;
            screen.backBufferHeight = params->BackBufferHeight;
            screen.colorDepth = 32;
            screen.driver = Screen::Driver::eDirectx9;

            return;
        }
        #endif

//        if ( strcmp( driverID, "OpenGL" ) == 0 )
//        {
//            screen.driver = Screen::Driver::eOpenGL;
//            return;
//        }


        screen.driver = Screen::Driver::eSoftware;

    }

	//..............................................................................


#if defined(BUILTIN_PLUGINS)
}
#endif
