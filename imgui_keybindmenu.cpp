/*
 * Prototype of a keybinding menu. Searching for "Doom3" should give you most of the places
 * in the code that need to be adjusted when integrating this in a real game.
 *
 * This code assumes that each key is bound to either one command (or action) or none at all.
 * IMHO this is a reasonable assumption to make (and true in Doom3), but if your game (or whatever)
 * allows binding the same key to multiple commands at the same time, that will require many changes.
 *
 * Based on ImGui's example code for SDL2 + classic OpenGL (because Doom3 uses OpenGL 1.4).
 * The keybinding menu doesn't interact with OpenGL directly, so that part is easy to change,
 * but I added some hacks in the SDL2-specific code (gamepadStartPressed and hadKeyDownEvent)
 * that must be ported for different input backends.
 *
 * To build, you also need the following source files from Dear ImGui 1.90.6 or 1.91.2 (other versions
 * might work, but those have been tested):
 *  imgui.cpp imgui_demo.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp
 *  backends/imgui_impl_sdl2.cpp backends/imgui_impl_opengl2.cpp
 * Furthermore you need to add the imgui root directory (that contains imgui.h) and its backends/
 * to the header-search directories (-I option in GCC/clang)
 * Or you just replace examples/example_sdl2_opengl2/main.cpp with this file and build and run that
 * example.
 *
 * (C) 2024 Daniel Gibson
 *
 * Released under MIT license, like Dear ImGui
 */

// Dear ImGui: standalone example application for SDL2 + OpenGL
// (SDL is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan/Metal graphics context creation, etc.)

// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

// **DO NOT USE THIS CODE IF YOUR CODE/ENGINE IS USING MODERN OPENGL (SHADERS, VBO, VAO, etc.)**
// **Prefer using the code in the example_sdl2_opengl3/ folder**
// See imgui_impl_sdl2.cpp for details.

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl2.h"
#include <stdio.h>
#include <SDL.h>
#include <SDL_opengl.h>

#include <string>
#include <vector>

// emulate idStr..
class idStr : public std::string {

public:

	idStr() = default;

	idStr(const char* s) : std::string(s) {}

	int Length() const {
		return (int)length();
	}

	operator const char* (void) const {
		return c_str();
	}

	static idStr		Format( const char* format, ... );
	static idStr		VFormat( const char* format, va_list argptr );
};

idStr idStr::Format( const char* format, ... )
{
	va_list argptr;
	va_start( argptr, format );
	idStr ret = VFormat( format, argptr );
	va_end( argptr );
	return ret;
}

idStr idStr::VFormat( const char* format, va_list argptr )
{
	idStr ret;
	int len;
	va_list argptrcopy;
	char buffer[16000];

	// make a copy of argptr in case we need to call vsnprintf() again after truncation
#ifdef va_copy // IIRC older VS versions didn't have this?
	va_copy( argptrcopy, argptr );
#else
	argptrcopy = argptr;
#endif

	len = vsnprintf( buffer, sizeof(buffer), format, argptr );

	if ( len < (int)sizeof(buffer) ) {
		ret = buffer;
	} else {
		// string was truncated, because buffer wasn't big enough.
		ret.resize( len );
		vsnprintf( &ret[0], len+1, format, argptrcopy );
	}
	va_end( argptrcopy );

	return ret;

}

// add tooltip with given text to the previously added widget (visible if that item is hovered)
static void AddTooltip( const char* text )
{
	if ( ImGui::BeginItemTooltip() )
	{
		ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
		ImGui::TextUnformatted( text );
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

// add a grey "(?)" that can be hovered for a tooltip with a description
// (or similar additional information)
static void AddDescrTooltip( const char* description )
{
	if ( description != nullptr ) {
		ImGui::SameLine();
		ImGui::TextDisabled( "(?)" );
		AddTooltip( description );
	}
}

// was there a key down or button (mouse/gamepad) down event this frame?
// used to make the warning overlay disappear
static bool hadKeyDownEvent = false;
// for special case in IsCancelKeyPressed()
static bool gamepadStartPressed = false;

static idStr warningOverlayText;
static double warningOverlayStartTime = -100.0;
static ImVec2 warningOverlayStartPos;

static float blaScale = 1.0f;

static void UpdateWarningOverlay()
{
	double timeNow = ImGui::GetTime();
	if ( timeNow - warningOverlayStartTime > 4.0f ) {
		return;
	}

	// also hide if a key was pressed or maybe even if the mouse was moved (too much)
	ImVec2 mdv = ImGui::GetMousePos() - warningOverlayStartPos; // Mouse Delta Vector
	float mouseDelta = sqrtf( mdv.x * mdv.x + mdv.y * mdv.y );
	const float fontSize = ImGui::GetFontSize();
	if ( mouseDelta > fontSize * 4.0f || hadKeyDownEvent ) {
		warningOverlayStartTime = -100.0f;
		return;
	}

	ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::PushStyleColor( ImGuiCol_WindowBg, ImVec4(1.0f, 0.4f, 0.4f, 0.4f) );
	float padSize = fontSize * 2.0f;
	ImGui::PushStyleVar( ImGuiStyleVar_WindowPadding, ImVec2(padSize, padSize) );

	int winFlags = ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove
			| ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize;
	ImGui::Begin("WarningOverlay", NULL, winFlags);

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 points[] = {
		{0, 40}, {40, 40}, {20, 0}, // triangle
		{20, 12}, {20, 28}, // line
		{20, 33} // dot
	};

	float iconScale = blaScale; // TODO: global scale also used for fontsize

	ImVec2 offset = ImGui::GetWindowPos() + ImVec2(fontSize, fontSize);
	for ( ImVec2& v : points ) {
		v.x = roundf( v.x * iconScale );
		v.y = roundf( v.y * iconScale );
		v += offset;
	}

	ImU32 color = ImGui::GetColorU32( ImVec4(0.1f, 0.1f, 0.1f, 1.0f) );
	//drawList->AddPolyline( points, 3, color, ImDrawFlags_RoundCornersAll | ImDrawFlags_Closed,
	//                       roundf( iconScale * 4.0f ) );
	drawList->AddTriangle( points[0], points[1], points[2], color, roundf( iconScale * 4.0f ) );

	drawList->AddPolyline( points+3, 2, color, 0, roundf( iconScale * 3.0f ) );

	float dotRadius = 2.0f * iconScale;
	drawList->AddEllipseFilled( points[5], ImVec2(dotRadius, dotRadius), color, 0, 6 );

	ImGui::Indent( 40.0f * iconScale );
	ImGui::TextUnformatted( warningOverlayText.c_str() );

	ImGui::End();

	ImGui::PopStyleVar(); // WindowPadding
	ImGui::PopStyleColor(); // WindowBg
}

static void ShowWarningOverlay( const char* text )
{
	warningOverlayText = text;
	warningOverlayStartTime = ImGui::GetTime();
	warningOverlayStartPos = ImGui::GetMousePos();
}


static bool IsKeyPressed( ImGuiKey key ) {
	return ImGui::IsKeyPressed( key, false );
}

// Enter or gamepad A, used to confirm or (when a binding table entry is focused/selected) to start binding a key
static bool IsConfirmKeyPressed() {
	return IsKeyPressed( ImGuiKey_Enter ) || IsKeyPressed( ImGuiKey_KeypadEnter )
	       || IsKeyPressed( ImGuiKey_GamepadFaceDown );
}

static bool IsClearKeyPressed() {
	return IsKeyPressed( ImGuiKey_Delete ) || IsKeyPressed( ImGuiKey_Backspace )
	       || IsKeyPressed( ImGuiKey_GamepadFaceUp );
}

static bool IsCancelKeyPressed() {
	// using Escape, gamepad Start and gamepad B for cancel, except in the
	// binding case (so gamepad B can be bound), there only Esc and Start work.
	// when the binding popup is active, gamepad nav is disabled, so
	//   IsKeyPressed( ImGuiKey_Gamepad* ) returns false.
	// That means that I need another way to detect gamepad Start being pressed for that case.
	// In this demo I'm using gamepadStartPressed, in Doom3/dhewm3 I'll likely ask Doom3 if it
	// thinks that Escape is pressed, becazse there gamepad start generates an Escape key event.

	// Note: In Doom3, Escape opens/closes the main menu, so in dhewm3 the gamepad Start button
	//       behaves the same, incl. the specialty that it can't be bound by the user
	return gamepadStartPressed || IsKeyPressed( ImGuiKey_Escape ) || IsKeyPressed( ImGuiKey_GamepadFaceRight );
		// || IsKeyPressed( ImGuiKey_GamepadStart );
}

static const char* GetGamepadStartName() {
	return "Start"; // TODO: get from Doom3, could be different depending on xbox vs PS vs nintendo
}

static const char* GetGamepadCancelButtonNames() {
	return "Start or B"; // TODO: get from Doom3, will be different depending on xbox vs PS vs nintendo
}

static const char* GetGamepadBindNowButtonName() { // TODO: rename to confirm or sth
	return "A"; // TODO: get from Doom3, will be different depending on xbox vs PS vs nintendo
}

static const char* GetGamepadUnbindButtonName() {
	return "Y"; // TODO: get from Doom3, ...
}

const char* GetKeyName( int keyNum, bool localized = true )
{
	if( keyNum <= 0 )
		return "<none>";
	// TODO: use Doom3's idKeyInput::KeyNumToString(keyNum, localized) instead
	// "localized" here not only means that the keyname might be translated, but also
	// that it's a name for displaying that might change with updates, while the not-localized
	// name is the internal name used in configs when storing key bindings that might be
	// less nice to read, but doesn't contain spaces and should not change with updates of dhewm3
	// (or wherever you integrate this code into)
	if ( localized ) {
		return ImGui::GetKeyName( (ImGuiKey)keyNum );
	} else {
		// prepend _int_ so I can tell internal and localized name apart
		// just a dummy, in Doom3 use idKeyInput::KeyNumToString(keyNum, false)
		static char internalKeyName[128];
		snprintf(internalKeyName, sizeof(internalKeyName), "_int_%s", ImGui::GetKeyName( (ImGuiKey)keyNum ) );
		return internalKeyName;
	}
}

// background color for the first column of the binding table, that contains the name of the command
static ImU32 displayNameBGColor = 0;

static const ImVec4 RedButtonColor(1.00f, 0.17f, 0.17f, 0.58f);
static const ImVec4 RedButtonHoveredColor(1.00f, 0.17f, 0.17f, 1.00f);
static const ImVec4 RedButtonActiveColor(1.00f, 0.37f, 0.37f, 1.00f);

static float CalcDialogButtonWidth()
{
	// with the standard font, 120px wide Ok/Cancel buttons look good,
	// this text (+default padding) has that width there
	float testTextWidth = ImGui::CalcTextSize( "Ok or Cancel ???" ).x;
	float framePadWidth = ImGui::GetStyle().FramePadding.x;
	return testTextWidth + 2.0f * framePadWidth;
}

enum BindingEntrySelectionState {
	BESS_NotSelected = 0,
	BESS_Selected,
	BESS_WantBind,
	BESS_WantClear,
	BESS_WantRebind // we were in WantBind, but the key is already bound to another command, so show a confirmation popup
};

struct BoundKey {
	int keyNum = -1;
	idStr keyName;
	idStr internalKeyName; // the one used in bind commands in the D3 console and config

	void Set( int _keyNum )
	{
		keyNum = _keyNum;
		keyName = GetKeyName( _keyNum );
		internalKeyName = GetKeyName( _keyNum, false );
	}

	void Clear()
	{
		keyNum = -1;
		keyName = "";
		internalKeyName = "";
	}

	BoundKey() = default;

	BoundKey ( int _keyNum ) {
		Set( _keyNum );
	}

};

struct BindingEntry;
static BindingEntry* FindBindingEntryForKey( int keyNum );
static int numBindingColumns = 4; // TODO: in Doom3, save in CVar (in_maxBindingsPerCommand or sth)

static int rebindKeyNum = -1; // only used for HandleRebindPopup()
static BindingEntry* rebindOtherEntry = nullptr; // ditto

struct BindingEntry {
	idStr command; // "_impulse3" or "_forward" or similar - or "" for heading entry
	idStr displayName;
	const char* description = nullptr;
	std::vector<BoundKey> bindings;
	enum {
		BIND_NONE   = -1, // no binding currently selected
		// all are selected (clicked command name column): clear all,
		//   or add a new binding in some visible column (idx in bindings < numBindingColumns)
		BIND_ALL    = -2,
		// append new binding at the end, or set it in unused bindings entry, if any (used by AllBindingsWindow)
		BIND_APPEND = -3
	};
	// which binding is currently selected in the UI, if any (and only if this
	//   binding entry is currently active according to Draw()'s oldSelState)
	int selectedBinding = BIND_NONE; // index in bindings or one of the enum values

	BindingEntry() = default;

	BindingEntry( const char* _displayName ) : displayName(_displayName) {}

	BindingEntry( const char* _command, const char* _displayName, const char* descr = nullptr )
	: command( _command ), displayName( _displayName ), description( descr ) {}

	// TODO: the following constructor is only relevant for the proof of concept code
	BindingEntry( const char* _command, const char* _displayName, const char* descr, std::initializer_list<BoundKey> boundKeys )
	: command( _command ), displayName( _displayName ), description( descr ), bindings( boundKeys ) {}

#if 0
	BindingEntry( const idStr& _command, const idStr& _displayName, const char* descr = nullptr )
		: command( _command ), displayName( _displayName ), description( descr ) {}

	BindingEntry( const idStr& _command, const char* _displayName, const char* descr = nullptr )
		: command( _command ), displayName( _displayName ), description( descr ) {}

	BindingEntry( const BindingEntryTemplate& bet )
	: command( bet.command ), description( bet.description ) {
		displayName = GetLocalizedString( bet.nameLocStr, bet.name );
		displayName.StripTrailingWhitespace();
	}
#endif

	bool IsHeading() const
	{
		return command.Length() == 0;
	}

	void Init()
	{
		// TODO: get bindings for command from Doom3
	}

	// only removes the entry from bindings, does *not* unbind!
	void RemoveBindingEntry( unsigned idx )
	{
		if ( idx < bindings.size() ) {
			auto it = bindings.begin();
			it += idx;
			bindings.erase( it );
		}
	}

	// remove all entries from bindings that don't have a key set
	void CompactBindings()
	{
		for ( int i = bindings.size() - 1; i >= 0; --i ) {
			if ( bindings[i].keyNum == -1 ) {
				RemoveBindingEntry( i );
			}
		}
	}

	// also updates this->selectedColumn
	void UpdateSelectionState( int bindIdx, /* in+out */ BindingEntrySelectionState& selState )
	{
		// if currently a popup is shown for creating a new binding or clearing one (BESS_WantBind
		// or BESS_WantClear), everything is still rendered, but in a disabled (greyed out) state
		// and shouldn't handle any input => then there's not much to do here,
		//  except for highlighting at the end of the function
		if ( selState < BESS_WantBind ) {
			if ( ImGui::IsItemFocused() ) {
				// Note: even when using the mouse, clicking a selectable will make it focused,
				//  so it's possible to select a command (or specific binding of a command)
				//  with the mouse and then press Enter to (re)bind it or Delete to clear it.
				//  So whether something is selected mostly is equivalent to it being focused.
				//  In the initial development of this code that wasn't the case, so there
				//  *might* be some small inconsistencies due to that; but also intentional
				//  special cases, like a binding entry being drawn as selected while
				//  one of the popups is open to modify it
				//  (=> it doesn't have focus then because the popup has focus)
				//  That's not just cosmetical, selState and selectedBinding are
				//  used to configure the popup.

				selectedBinding = bindIdx;

				if ( IsConfirmKeyPressed() ) {
					printf("focus bind now bindIdx %d selected_Binding %d\n", bindIdx, selectedBinding);
					selState = BESS_WantBind;
				} else if ( IsClearKeyPressed() ) {
					printf("focus clear now bindIdx %d selected_Binding %d\n", bindIdx, selectedBinding);
					bool nothingToClear = false;
					if ( bindIdx == BIND_ALL ) {
						if ( bindings.size() == 0 ) {
							ShowWarningOverlay( "No keys are bound to this command, so there's nothing to unbind" );
							nothingToClear = true;
						}
					} else if ( bindIdx < 0 || bindIdx >= (int)bindings.size() || bindings[bindIdx].keyNum == -1 ) {
						ShowWarningOverlay( "No bound key selected for unbind" );
						nothingToClear = true;
					}

					selState = nothingToClear ? BESS_Selected : BESS_WantClear;
				} else if ( selState == BESS_NotSelected ) {
					printf("focus select %s bindIdx %d selected_Binding %d\n", displayName.c_str(), bindIdx, selectedBinding);
					selState = BESS_Selected;
				}
			} else if (selectedBinding == bindIdx && selState != BESS_NotSelected) {
				// apparently this was still selected last frame, but is not focused anymore => unselect it
				printf("unselect %s %d from lack of focus\n", displayName.c_str(), bindIdx);
				selState = BESS_NotSelected;
			}

			if ( ImGui::IsItemHovered() ) { // mouse cursor is on this item
				if ( bindIdx == BIND_ALL ) {
					// if the first column (command name, like "Move Left") is hovered, highlight the whole row
					// A normal Selectable would use ImGuiCol_HeaderHovered, but I use that as the "selected"
					// color (in Draw()), so use the next brighter thing (ImGuiCol_HeaderActive) here.
					ImU32 highlightRowColor = ImGui::GetColorU32( ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive) );
					ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, highlightRowColor );
				}

				if ( ImGui::IsMouseDoubleClicked( 0 ) ) {
					printf("hover doubleclick bindIdx %d selected_Binding %d\n", bindIdx, selectedBinding);
					selState = BESS_WantBind;
					selectedBinding = bindIdx;
				}
				// Note: single-clicking an item gives it focus, so that's implictly
				//  handled above in `if ( ImGui::IsItemFocused() ) { ...`
			}
		}

		// this column is selected => highlight it
		if ( selState != BESS_NotSelected && selectedBinding == bindIdx ) {
			// ImGuiCol_Header would be the regular "selected cell/row" color that Selectable would use
			// but ImGuiCol_HeaderHovered is more visible, IMO
			ImU32 highlightRowColor = ImGui::GetColorU32( ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered) );
			ImGui::TableSetBgColor( ImGuiTableBgTarget_CellBg, highlightRowColor );

			if ( bindIdx == BIND_ALL ) {
				// the displayName column is selected => highlight the whole row
				ImGui::TableSetBgColor( ImGuiTableBgTarget_RowBg0, highlightRowColor );
				// (yes, still set the highlight color for ImGuiTableBgTarget_CellBg above for extra
				//  highlighting of the column 0 cell, otherwise it'd look darker due to displayNameBGColor)
			}
		}

		//printf("XX returning from UpdateSelectionState() %s selectedBinding = %d selState = %d\n", displayName.c_str(), selectedBinding, selState);
	}

	bool DrawAllBindingsWindow( /* in+out */ BindingEntrySelectionState& selState, bool newOpen, const ImVec2& btnMin, const ImVec2& btnMax )
	{
		bool showThisMenu = true;
		idStr menuWinTitle = idStr::Format( "All keys bound to %s###allBindingsWindow", displayName.c_str() );
		int numBindings = bindings.size();

		ImGuiWindowFlags menuWinFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings;
		const float fontSize = ImGui::GetFontSize();
		ImVec2 winMinSize = ImGui::CalcTextSize( menuWinTitle, nullptr, true );
		winMinSize.x += fontSize * 2.0f;
		const ImGuiViewport& viewPort = *ImGui::GetMainViewport();
		ImVec2 maxWinSize( viewPort.WorkSize.x, viewPort.WorkSize.y * 0.9f );
		// make sure the window is big enough to show the full title (incl. displayName)
		// and that it fits into the screen (it can scroll if it gets too long)
		ImGui::SetNextWindowSizeConstraints( winMinSize, maxWinSize );

		static ImVec2 winPos;
		if ( newOpen ) {
			// position the window right next to the [++] button that opens/closes it
			winPos = btnMin;
			winPos.x = btnMax.x + ImGui::GetStyle().ItemInnerSpacing.x;
			ImGui::OpenPopup( menuWinTitle );
			ImGui::SetNextWindowPos(winPos);
			ImGui::SetNextWindowFocus();
		}

		if ( ImGui::Begin( menuWinTitle, &showThisMenu, menuWinFlags ) )
		{
			ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg;
			if ( numBindings > 0 && ImGui::BeginTable( "AllBindingsForCommand", 2, tableFlags ) ) {
				ImGui::TableSetupColumn("command", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("buttons", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex( 0 );

				// turn the next button (Unbind all) red
				ImGui::PushStyleColor( ImGuiCol_Button, RedButtonColor );
				ImGui::PushStyleColor( ImGuiCol_ButtonHovered, RedButtonHoveredColor );
				ImGui::PushStyleColor( ImGuiCol_ButtonActive,  RedButtonActiveColor );

				ImGui::Indent();
				if ( ImGui::Button( idStr::Format( "Unbind all" ) ) ) {
					selState = BESS_WantClear;
					selectedBinding = BIND_ALL;
				} else {
					ImGui::SetItemTooltip( "Remove all keybindings for %s", displayName.c_str() );
				}

				ImGui::Unindent();
				ImGui::PopStyleColor(3); // return to normal button color

				ImGui::TableSetColumnIndex( 1 );

				float helpHoverWidth = ImGui::CalcTextSize("(?)").x;
				float offset = ImGui::GetContentRegionAvail().x - helpHoverWidth;
				ImGui::SetCursorPosX( ImGui::GetCursorPosX() + offset );
				ImGui::AlignTextToFramePadding();
				ImGui::TextDisabled( "(?)" );
				if ( ImGui::BeginItemTooltip() ) {
					ImGui::PushTextWrapPos( ImGui::GetFontSize() * 35.0f );
					ImGui::Text( "You can close this window with Escape or %s on the gamepad or by clicking the little (x) button or by clicking the [++] button again.",
					             GetGamepadCancelButtonNames() );
					ImGui::PopTextWrapPos();
					ImGui::EndTooltip();
				}

				ImGui::Spacing();

				ImU32 highlightRowColor = 0;
				if ( selectedBinding == BIND_ALL ) {
					highlightRowColor = ImGui::GetColorU32( ImGui::GetStyleColorVec4(ImGuiCol_HeaderHovered) );
				}

				ImGui::Indent( fontSize * 0.5f );

				for ( int bnd = 0; bnd < numBindings; ++bnd ) {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex( 0 );

					ImGui::PushID( bnd ); // the buttons have the same names in every row, so push the row number as ID

					bool colHasBinding = bindings[bnd].keyNum != -1;
					const char* keyName = "";

					if ( colHasBinding ) {
						keyName = bindings[bnd].keyName.c_str();
						ImGui::AlignTextToFramePadding();
						ImGui::TextUnformatted( keyName );
						AddTooltip( bindings[bnd].internalKeyName.c_str() );
					}

					if ( selectedBinding == BIND_ALL ) {
						// if all bindings are selected from the binding table (for clear all),
						// mark the whole first row here. otherwise, nothing is marked here,
						// as in this window only the buttons are clickble, not the key cells
						ImGui::TableSetBgColor( ImGuiTableBgTarget_CellBg, highlightRowColor );
					}

					ImGui::TableNextColumn();
					if ( colHasBinding ) {
						if ( ImGui::Button( "Rebind" ) ) {
							selState = BESS_WantBind;
							selectedBinding = bnd;
						} else {
							ImGui::SetItemTooltip( "Unbind '%s' and bind another key to %s", keyName, displayName.c_str() );
						}
						ImGui::SameLine();
						ImGui::SetCursorPosX( ImGui::GetCursorPosX() + fontSize*0.5f );
						if ( ImGui::Button( "Unbind" ) ) {
							selState = BESS_WantClear;
							selectedBinding = bnd;
						} else {
							ImGui::SetItemTooltip( "Unbind key '%s' from %s", keyName, displayName.c_str() );
						}
					} else {
						if ( ImGui::Button( "Bind" ) ) {
							selState = BESS_WantBind;
							selectedBinding = bnd;
						} else {
							ImGui::SetItemTooltip( "Set a keybinding for %s", displayName.c_str() );
						}
					}

					ImGui::PopID(); // bnd
				}

				ImGui::EndTable();
			}

			const char* addBindButtonLabel = (numBindings == 0) ? "Bind a key" : "Bind another key";
			float buttonWidth = ImGui::CalcTextSize(addBindButtonLabel).x + 2.0f * ImGui::GetStyle().FramePadding.x;
			ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - buttonWidth);

			if ( ImGui::Button( addBindButtonLabel ) ) {
				selState = BESS_WantBind;
				selectedBinding = BIND_APPEND;
			} else {
				ImGui::SetItemTooltip( "Add %s keybinding for %s",
									   (numBindings == 0) ? "a" : "another",
									   displayName.c_str() );
			}

			ImVec2 winSize = ImGui::GetWindowSize();
			ImRect winRect;
			winRect.Min = ImGui::GetWindowPos();
			winRect.Max = winRect.Min + winSize;
			ImRect workRect(viewPort.WorkPos, viewPort.WorkPos + viewPort.WorkSize);

			if ( !workRect.Contains(winRect) ) {
				// this window is at least partly outside the visible area of the screen (or SDL window)
				// => move it around so it's completely visible, if possible
				ImRect r_avoid( btnMin, btnMax );
				r_avoid.Expand( ImGui::GetStyle().ItemInnerSpacing );

				ImGuiDir dir = ImGuiDir_Right;
				ImVec2 newWinPos = ImGui::FindBestWindowPosForPopupEx( ImVec2(btnMin.x, btnMax.y),
							winSize, &dir, workRect, r_avoid, ImGuiPopupPositionPolicy_Default );
				ImVec2 posDiff = newWinPos - winPos;
				if ( fabsf(posDiff.x) > 2.0f || fabsf(posDiff.y) > 2.0f ) {
					winPos = newWinPos;
					ImGui::SetWindowPos( newWinPos );
				}
			}

			// allow closing this window with escape
			if ( ImGui::IsWindowFocused() && IsCancelKeyPressed() ) {
				showThisMenu = false;
			}
		}
		ImGui::End();

		return showThisMenu;
	}

	BindingEntrySelectionState Draw( int bindRowNum, const BindingEntrySelectionState oldSelState )
	{
		if ( IsHeading() ) {
			ImGui::SeparatorText( displayName );
			if ( description ) {
				AddDescrTooltip( description );
			}
		} else {
			ImGui::PushID( command );

			ImGui::TableNextRow( 0, ImGui::GetFrameHeightWithSpacing() );
			ImGui::TableSetColumnIndex( 0 );
			ImGui::AlignTextToFramePadding();

			// the first column (with the display name in it) gets a different background color
			ImGui::TableSetBgColor( ImGuiTableBgTarget_CellBg, displayNameBGColor );

			BindingEntrySelectionState newSelState = oldSelState;

			// not really using the selectable feature, mostly making it selectable
			// so keyboard/gamepad navigation works
			ImGui::Selectable( "##cmd", false, 0 );

			UpdateSelectionState( BIND_ALL, newSelState );

			ImGui::SameLine();
			ImGui::TextUnformatted( displayName );

			AddTooltip( command );

			if ( description ) {
				AddDescrTooltip( description );
			}

			int numBindings = bindings.size();
			for ( int bnd=0; bnd < numBindingColumns ; ++bnd ) {
				ImGui::TableSetColumnIndex( bnd+1 );

				bool colHasBinding = (bnd < numBindings) && bindings[bnd].keyNum != -1;
				char selTxt[128];
				if ( colHasBinding ) {
					snprintf( selTxt, sizeof(selTxt), "%s###%d", bindings[bnd].keyName.c_str(), bnd );
				} else {
					snprintf( selTxt, sizeof(selTxt), "###%d", bnd );
				}
				ImGui::Selectable( selTxt, false, 0 );
				UpdateSelectionState( bnd, newSelState );

				if ( colHasBinding ) {
					AddTooltip( bindings[bnd].internalKeyName.c_str() );
				}
			}

			ImGui::TableSetColumnIndex( numBindingColumns + 1 );
			// the last column contains a "++" button that opens a window that lists all bindings
			// for this rows command (including ones not listed in the table because of lack of columns)
			// if there actually are more bindings than columns, the button is red, else it has the default color
			// clicking the button again will close the window, and the buttons color depends on whether
			// its window is open or not. only one such window can be open at at time, clicking the
			// button in another row closes the current window and opens a new one
			static int  showAllBindingsMenuRowNum = -1;
			bool allBindWinWasOpen = (showAllBindingsMenuRowNum == bindRowNum);
			int styleColorsToPop = 0;
			if ( numBindings <= numBindingColumns ) {
				if ( allBindWinWasOpen ) {
					// if the all bindings menu/window is showed for this entry,
					// the button is "active" => switch its normal and hovered colors
					ImVec4 btnColor = ImGui::GetStyleColorVec4( ImGuiCol_ButtonHovered );
					ImGui::PushStyleColor( ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_Button) );
					ImGui::PushStyleColor( ImGuiCol_Button, btnColor );
					styleColorsToPop = 2;
				}
			} else { // more bindings than can be shown in the table => make ++ button red
				ImGui::PushStyleColor( ImGuiCol_Button, allBindWinWasOpen ? RedButtonHoveredColor : RedButtonColor );
				ImGui::PushStyleColor( ImGuiCol_ButtonHovered, allBindWinWasOpen ? RedButtonColor : RedButtonHoveredColor );
				ImGui::PushStyleColor( ImGuiCol_ButtonActive, RedButtonActiveColor );
				styleColorsToPop = 3;
			}

			// TODO: close the window if another row has been selected (or used to create/delete a binding or whatever)?
			bool newOpen = false;
			if ( ImGui::Button( "++" ) ) {
				showAllBindingsMenuRowNum = allBindWinWasOpen ? -1 : bindRowNum;
				newOpen = true;
				CompactBindings();
			}
			if ( ImGui::IsItemFocused() && newSelState != BESS_NotSelected ) {
				printf("> Draw(): unselect because ++ button seems to be focused\n");
				newSelState = BESS_NotSelected;
			}
			ImVec2 btnMin = ImGui::GetItemRectMin();
			ImVec2 btnMax = ImGui::GetItemRectMax();
			if ( numBindings > numBindingColumns ) {
				ImGui::SetItemTooltip( "There are additional bindings for %s.\nClick here to show all its bindings.", displayName.c_str() );
			} else {
				ImGui::SetItemTooltip( "Show all bindings for %s in a list", displayName.c_str() );
			}

			if ( styleColorsToPop > 0 ) {
				ImGui::PopStyleColor( styleColorsToPop ); // restore button colors
			}

			if ( showAllBindingsMenuRowNum == bindRowNum ) {
				ImGui::SetNextWindowBgAlpha( 1.0f );
				if ( !DrawAllBindingsWindow( newSelState, newOpen, btnMin, btnMax ) ) {
					showAllBindingsMenuRowNum = -1;
					CompactBindings();
				}
			}

			ImGui::PopID();

			if ( newSelState == BESS_NotSelected ) {
				if(oldSelState != newSelState)
					printf("> Draw(): new state of %s is not selected\n", displayName.c_str());
				selectedBinding = BIND_NONE;
			}
			return newSelState;
		}
		return BESS_NotSelected;
	}

	void Bind( int keyNum ) {
		if ( keyNum >= 0 ) {
			printf( "bind key %d to %s (%s)\n", keyNum, command.c_str(), displayName.c_str() );
			// TODO: actual Doom3 bind implementation!
		}
	}

	void Unbind( int keyNum ) {
		if ( keyNum >= 0 ) {
			printf( "unbind key %d from %s (%s)\n", keyNum, command.c_str(), displayName.c_str() );
			// TODO: actual Doom3 unbind implementation!
		}
	}

	BindingEntrySelectionState HandleClearPopup( const char* popupName, bool newOpen )
	{
		BindingEntrySelectionState ret = BESS_WantClear;
		const int selectedBinding = this->selectedBinding;

		if ( ImGui::BeginPopupModal( popupName, NULL, ImGuiWindowFlags_AlwaysAutoResize ) )
		{
			if ( selectedBinding == BIND_ALL ) {
				ImGui::Text( "Clear all keybindings for %s ?", displayName.c_str() );
			} else {
				ImGui::Text( "Unbind key '%s' from command %s ?",
				             bindings[selectedBinding].keyName.c_str(), displayName.c_str() );
			}

			ImGui::NewLine();
			ImGui::Text( "Press Enter (or gamepad %s) to confirm, or\nEscape (or gamepad %s) to cancel.",
			             GetGamepadBindNowButtonName(), GetGamepadCancelButtonNames() );
			ImGui::NewLine();

			// center the Ok and Cancel buttons
			float dialogButtonWidth = CalcDialogButtonWidth();
			float spaceBetweenButtons = ImGui::GetFontSize();
			float buttonOffset = (ImGui::GetWindowWidth() - 2.0f*dialogButtonWidth - spaceBetweenButtons) * 0.5f;
			ImGui::SetCursorPosX( buttonOffset );

			bool confirmedByKey = false;
			if ( !newOpen && !ImGui::IsAnyItemFocused() ) {
				// if no item is focused (=> not using keyboard or gamepad navigation to select
				//  [Ok] or [Cancel] button), check if Enter has been pressed to confirm deletion
				// (otherwise, enter can be used to chose the selected button)
				// also, don't do this when just opened, because then enter might still
				// be pressed from selecting the Unbind button in the AllBindingsWindow
				confirmedByKey = IsConfirmKeyPressed();
			}
			if ( ImGui::Button( "Ok", ImVec2(dialogButtonWidth, 0) ) || confirmedByKey ) {
				if ( selectedBinding == BIND_ALL ) {
					for ( BoundKey& bk : bindings ) {
						Unbind( bk.keyNum );
					}
					bindings.clear();
					// don't select all columns after they have been cleared,
					// instead only select the first, good point to add a new binding
					this->selectedBinding = 0;
				} else {
					Unbind( bindings[selectedBinding].keyNum );
					if ( selectedBinding == numBindingColumns - 1 ) {
						// when removing the binding of the last column visible
						// in the big binding table, remove that entry so
						// the next one not visible there can take its place
						RemoveBindingEntry( selectedBinding );
					} else {
						bindings[selectedBinding].Clear();
					}
				}

				ImGui::CloseCurrentPopup();
				ret = BESS_Selected;
			}
			ImGui::SetItemDefaultFocus();

			ImGui::SameLine( 0.0f, spaceBetweenButtons );
			if ( ImGui::Button( "Cancel", ImVec2(dialogButtonWidth, 0) ) || IsCancelKeyPressed() ) {
				ImGui::CloseCurrentPopup();
				ret = BESS_Selected;
			}

			ImGui::EndPopup();
		}

		return ret;
	}


	void AddKeyBinding( int keyNum )
	{
		assert( selectedBinding != -1 );
		Bind( keyNum );

		int numBindings = bindings.size();
		if ( selectedBinding == BIND_ALL || selectedBinding == BIND_APPEND ) {
			for ( int i=0; i < numBindings; ++i ) {
				// if there's an empty column, use that
				if ( bindings[i].keyNum == -1 ) {
					bindings[i].Set( keyNum );
					// select the column this was inserted into
					selectedBinding = i;

					return;
				}
			}
			if ( numBindings < numBindingColumns || selectedBinding == BIND_APPEND ) {
				// just append an entry to bindings
				bindings.push_back( BoundKey(keyNum) );
				selectedBinding = numBindings;
			} else {
				// insert in last column of table so it's visible
				// (but don't remove any elements from bindings!)
				auto it = bindings.cbegin(); // I fucking hate STL.
				it += (numBindingColumns-1);
				bindings.insert( it, BoundKey(keyNum) );
				selectedBinding = numBindingColumns-1;
			}
		} else {
			int selectedBinding = this->selectedBinding;
			assert( selectedBinding >= 0 );
			if ( selectedBinding < numBindings ) {
				Unbind( bindings[selectedBinding].keyNum );
				bindings[selectedBinding].Set( keyNum );
			} else  {
				if ( selectedBinding > numBindings ) {
					// apparently a column with other unset columns before it was selected
					// => add enough empty columns
					bindings.resize( selectedBinding );
				}
				bindings.push_back( BoundKey(keyNum) );
			}
		}
	}

	void RemoveKeyBinding( int keyNum )
	{
		int delPos = -1;
		int numBindings = bindings.size();
		for ( int i = 0; i < numBindings; ++i ) {
			if ( bindings[i].keyNum == keyNum ) {
				delPos = i;
				break;
			}
		}
		if ( delPos != -1 ) {
			Unbind( keyNum );
			RemoveBindingEntry( delPos );
		}
	}


	BindingEntrySelectionState HandleBindPopup( const char* popupName, bool newOpen )
	{
		BindingEntrySelectionState ret = BESS_WantBind;
		const int selectedBinding = this->selectedBinding;
		assert(selectedBinding == BIND_ALL || selectedBinding == BIND_APPEND || selectedBinding >= 0);

		ImGuiIO& io = ImGui::GetIO();
		// disable keyboard and gamepad input while the bind popup is open
		io.ConfigFlags &= ~(ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard);

		if ( ImGui::BeginPopupModal( popupName, NULL, ImGuiWindowFlags_AlwaysAutoResize ) )
		{
			if ( selectedBinding < 0 || selectedBinding >= (int)bindings.size()
			    || bindings[selectedBinding].keyNum == -1 ) {
				// add a binding
				ImGui::Text( "Press a key or button to bind to %s", displayName.c_str() );
			} else {
				// overwrite a binding
				ImGui::Text( "Press a key or button to replace '%s' binding to %s",
				              bindings[selectedBinding].keyName.c_str(), displayName.c_str() );
			}

			ImGui::NewLine();
			ImGui::TextUnformatted( "To bind a mouse button, click it in the following field" );

			const float windowWidth = ImGui::GetWindowWidth();
			ImVec2 clickFieldSize( windowWidth * 0.8f, ImGui::GetTextLineHeightWithSpacing() * 4.0f );
			ImGui::SetCursorPosX( windowWidth * 0.1f );

			ImGui::Button( "###clickField", clickFieldSize );
			bool clickFieldHovered = ImGui::IsItemHovered();

			ImGui::NewLine();
			ImGui::Text( "... or press Escape (or gamepad %s) to cancel.", GetGamepadStartName() );

			ImGui::NewLine();
			// center the Cancel button
			float dialogButtonWidth = CalcDialogButtonWidth();
			float buttonOffset = (windowWidth - dialogButtonWidth) * 0.5f;
			ImGui::SetCursorPosX( buttonOffset );

			if ( ImGui::Button( "Cancel", ImVec2(dialogButtonWidth, 0) ) || IsCancelKeyPressed() ) {
				ImGui::CloseCurrentPopup();
				ret = BESS_Selected;
				io.ConfigFlags |= (ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard);
			} else if ( !newOpen ) {
				// find out if any key is pressed and bind that (except for Esc which can't be
				// bound and is already handled though IsCancelKeyPressed() above)
				// (but don't run this when the popup has just been opened, because then
				//  the key that opened this, likely Enter, is still registered as pressed)

				// TODO: use Doom3's mechanism to figure out what D3 key is pressed
				ImGuiKey pressedKey = ImGuiKey_None;
				for ( int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; ++k ) {
					ImGuiKey key = (ImGuiKey)k;
					if ( key >= ImGuiKey_MouseLeft && key <= ImGuiKey_MouseMiddle && !clickFieldHovered ) {
						// ignore mouse buttons, unless they clicked the clickField
						continue;
					}

					if ( IsKeyPressed( key ) ) {
						pressedKey = key;
						break;
					}

					// Note: in the standalone demo this doesn't work for gamepad buttons
					//   (because I disabled ImGuiConfigFlags_NavEnableGamepad, and
					//    the SDL2 backend currently completely disables gamepad input then,
					//    see "FIXME: Technically feeding gamepad shouldn't depend on this now that they are regular inputs."
					//    in backends/imgui_impl_sdl2.cpp)
					//   but in Doom3 I'll use Doom3 key events for this,
					//   so it will work there either way.
				}

				if ( pressedKey != ImGuiKey_None ) {
					// FIXME: also check if it's bound to some other (custom) Doom3 command
					BindingEntry* oldBE = FindBindingEntryForKey( pressedKey );
					if ( oldBE == nullptr ) {
						// that key isn't bound yet, hooray!
						AddKeyBinding( pressedKey );
						ret = BESS_Selected;
					} else if ( oldBE == this ) {
						// that key is already bound to this command, show warning, otherwise do nothing
						const char* keyName = GetKeyName( pressedKey );
						idStr warning = idStr::Format( "Key '%s' is already bound to this command (%s)!",
						                               keyName, displayName.c_str() );
						ShowWarningOverlay( warning );
						ret = BESS_Selected;
						// TODO: select column with that specific binding?
					} else {
						// that key is already bound to some other command, show confirmation popup
						// FIXME: must also handle the case that it's binding to some Doom3 command that is *not* handled by the menu
						rebindKeyNum = pressedKey;
						rebindOtherEntry = oldBE;

						ret = BESS_WantRebind;
					}
					ImGui::CloseCurrentPopup();
					io.ConfigFlags |= (ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard);
				}
			}
			ImGui::EndPopup();
		}

		return ret;
	}

	BindingEntrySelectionState HandleRebindPopup( const char* popupName, bool newOpen )
	{
		BindingEntrySelectionState ret = BESS_WantRebind;

		if ( ImGui::BeginPopupModal( popupName, NULL, ImGuiWindowFlags_AlwaysAutoResize ) )
		{
			const char* keyName = GetKeyName( rebindKeyNum );

			// TODO: if rebindOtherEntry == NULL, get command for rebindKeyNum from Doom3 directly
			//       and don't use rebindOtherEntry !

			ImGui::Text( "Key '%s' is already bound to command %s !\nBind to %s instead?",
			             keyName, rebindOtherEntry->displayName.c_str(), displayName.c_str() );
			ImGui::NewLine();
			ImGui::Text( "Press Enter (or gamepad %s) to confirm,\nor Escape (or gamepad %s) to cancel.",
			             GetGamepadBindNowButtonName(), GetGamepadCancelButtonNames() );
			ImGui::NewLine();

			// center the Ok and Cancel buttons
			float dialogButtonWidth = CalcDialogButtonWidth();
			float spaceBetweenButtons = ImGui::GetFontSize();
			float buttonOffset = (ImGui::GetWindowWidth() - 2.0f*dialogButtonWidth - spaceBetweenButtons) * 0.5f;
			ImGui::SetCursorPosX( buttonOffset );

			bool confirmedByKey = false;
			if ( !newOpen && !ImGui::IsAnyItemFocused() ) {
				// if no item is focused (=> not using keyboard or gamepad navigation to select
				//  [Ok] or [Cancel] button), check if Enter has been pressed to confirm deletion
				// (otherwise, enter can be used to chose the selected button)
				// but don't do this when just opened, because then enter might still
				// be pressed from trying to bind Enter in the BindPopup
				confirmedByKey = IsConfirmKeyPressed();
			}

			if ( ImGui::Button( "Ok", ImVec2(dialogButtonWidth, 0) ) || confirmedByKey ) {
				rebindOtherEntry->RemoveKeyBinding( rebindKeyNum );
				AddKeyBinding( rebindKeyNum );

				rebindOtherEntry = nullptr;
				rebindKeyNum = -1;

				ImGui::CloseCurrentPopup();
				ret = BESS_Selected;
			}
			ImGui::SetItemDefaultFocus();

			ImGui::SameLine( 0.0f, spaceBetweenButtons );
			if ( ImGui::Button( "Cancel", ImVec2(dialogButtonWidth, 0) ) || IsCancelKeyPressed() ) {
				rebindOtherEntry = nullptr;
				rebindKeyNum = -1;

				ImGui::CloseCurrentPopup();
				ret = BESS_Selected;
			}

			ImGui::EndPopup();
		}

		return ret;
	}

	void HandlePopup( BindingEntrySelectionState& selectionState )
	{
		assert(selectedBinding != BIND_NONE);
		const char* popupName = nullptr;

		if ( selectionState == BESS_WantClear ) {
			if ( bindings.size() == 0 || selectedBinding >= (int)bindings.size()
			    || ( selectedBinding < 0 && selectedBinding != BIND_ALL )
			    || ( selectedBinding >= 0 && bindings[selectedBinding].keyNum == -1 ) ) {
				// there are no bindings at all for this command, or at least not in the selected column
				// => don't show popup, but keep the cell selected
				selectionState = BESS_Selected;
				return;
			}
			popupName = (selectedBinding == BIND_ALL) ? "Unbind keys" : "Unbind key";
		} else if ( selectionState == BESS_WantBind ) {
			popupName = "Bind key";
		} else {
			assert( selectionState == BESS_WantRebind );
			popupName = "Confirm rebinding key";
		}

		static bool popupOpened = false;
		bool newOpen = false;
		if ( !popupOpened ) {
			ImGui::OpenPopup( popupName );
			popupOpened = true;
			newOpen = true;
		}
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos( center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f) );

		BindingEntrySelectionState newSelState = BESS_Selected;
		if ( selectionState == BESS_WantClear ) {
			newSelState = HandleClearPopup( popupName, newOpen );
		} else if ( selectionState == BESS_WantBind ) {
			newSelState = HandleBindPopup( popupName, newOpen );
		} else {
			newSelState = HandleRebindPopup( popupName, newOpen );
		}

		if ( newSelState != selectionState ) {
			popupOpened = false;
			selectionState = newSelState;
		}
	}
};

// in Doom3 there's a dynamic array of bindingEntries that's generated based on weapons available in
// the current game mode etc when the menu is first opened, this is just demo data to fill the menu
static BindingEntry bindingEntries[] = {
	{ "", "Move / Look", nullptr },
	{ "_forward",       "Forward"    , nullptr, { ImGuiKey_W, ImGuiKey_GamepadLStickUp, ImGuiKey_E, ImGuiKey_R, ImGuiKey_Z, ImGuiKey_T } },
	{ "_back",          "Backpedal"  , "walk back", {ImGuiKey_S} },
#if 01
	{ "_moveLeft",      "Move Left"  , "strafe left" },
	{ "_moveRight",     "Move Right" , nullptr },
	{ "", "Weapons", nullptr },
	{ "_impulse0",      "Fists",  "the other kind of fisting" },
	{ "_impulse1",      "Pistol",  nullptr },
#endif
};

// return NULL if not currently bound to anything
static BindingEntry* FindBindingEntryForKey( int keyNum )
{
	for ( BindingEntry& be : bindingEntries ) {
		for ( const BoundKey& bk : be.bindings ) {
			if ( bk.keyNum == keyNum ) {
				return &be;
			}
		}
	}
	return nullptr;
}

static void DrawBindingsMenu()
{
	{
		// the InputInt will look kinda like this:
		// [10] [-] [+] Number of Binding columns...
		// the [-] and [+] buttons have size GetFrameHeight()^2
		// calculate the width it needs (without the "Number of ..." text)
		// for the text input field not too look too big
		const ImGuiStyle& st = ImGui::GetStyle();
		float w = ImGui::CalcTextSize("10").x;
		w += 2.0f * (ImGui::GetFrameHeight() + st.FramePadding.x + st.ItemInnerSpacing.x);
		ImGui::SetNextItemWidth(w);

		ImGui::InputInt( "Number of Binding columns to show", &numBindingColumns );

		if ( numBindingColumns < 1 )
			numBindingColumns = 1;
		else if ( numBindingColumns > 10 )
			numBindingColumns = 10;

		// calculate the background color for the first column of the key binding tables
		// (it contains the command, while the other columns contain the keys bound to that command)
		ImVec4 bgColor = ImGui::GetStyleColorVec4( ImGuiCol_TableHeaderBg );
		bgColor.w = 0.5f;
		displayNameBGColor = ImGui::GetColorU32( bgColor );


		if ( ImGui::TreeNode("Usage Help") ) {
			AddTooltip( "Click to hide help text" );
			ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
			const char* gamepadBindNowButton = GetGamepadBindNowButtonName();
			const char* gamepadDeleteNowButton = GetGamepadUnbindButtonName();
			const char* gamepadCancelButton = GetGamepadCancelButtonNames();
			ImGui::TextWrapped( "Double click a keybinding entry below to bind a (different) key, or select it by clicking it once or navigating to it with cursor keys or gamepad and pressing Enter (or gamepad %s) to (re)bind it.",
			                    gamepadBindNowButton );
			ImGui::TextWrapped( "Remove a key binding (unbind) by selecting it and pressing Backspace, Delete or %s.",
			                    gamepadDeleteNowButton );
			ImGui::TextWrapped( "If you select the first column (with the command name), you can unbind all keybindings for that command, or add another keybinding for it without overwriting an existing one." );
			ImGui::TextWrapped( "You can unselect the currently selected binding by clicking it again or by pressing Escape or gamepad %s.",
			                    gamepadCancelButton );
			ImGui::TextWrapped( "The [++] button on the right opens (or closes) a window that shows all keys bound to the corresponding command (even if it's more than the amount of binding columns) and has buttons to configure them. It's red when there actually are more key bound than can be shown in the columns of this window." );

			ImGui::PopStyleColor(); // ImGuiCol_Text
			ImGui::TreePop();
		} else {
			AddTooltip( "Click to show help text" );
		}
	}

	static int selectedRow = -1;
	static BindingEntrySelectionState selectionState = BESS_NotSelected;

	// make the key column entries in the bindings table center-aligned instead of left-aligned
	ImGui::PushStyleVar( ImGuiStyleVar_SelectableTextAlign, ImVec2(0.5f, 0.0f) );

	// make vertical lines in the table less opaque
	ImU32 borderCol = ImGui::GetColorU32( ImGuiCol_TableBorderLight, 0.5f );
	ImGui::PushStyleColor( ImGuiCol_TableBorderLight, borderCol );

	ImGuiTableFlags tableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV;
	// inTable: are we currently adding elements to a table of bindings?
	//  (we're not when adding a heading from bindingEntries: existing tables are ended
	//   before a heading and a new table is started afterwards)
	bool inTable = false;
	// did the last ImGui::BeginTable() call return true (or is it not currently visible)?
	// (init to true so the first heading before any bindings is shown)
	bool lastBeginTable = true;
	int tableNum = 1;

	const ImVec2 defFramePadding = ImGui::GetStyle().FramePadding;
	const float commandColumnWidth = ImGui::CalcTextSize( "dhewm3 settings menu" ).x;
	const float overflowColumnWidth = ImGui::CalcTextSize( "++" ).x + defFramePadding.x * 2.0f;

	const int numBindingEntries = IM_ARRAYSIZE(bindingEntries); // FIXME: adjust for doom3
	for ( int i=0; i < numBindingEntries; ++i ) {
		BindingEntry& be = bindingEntries[i];
		bool isHeading = be.IsHeading();
		if ( !isHeading && !inTable ) {
			// there is no WIP table (that has been started but not ended yet)
			// and the current element is a regular bind entry that's supposed
			// to go in a table, so start a new one
			inTable = true;
			char tableID[10];
			snprintf( tableID, sizeof(tableID), "bindTab%d", tableNum++ );
			lastBeginTable = ImGui::BeginTable( tableID, numBindingColumns + 2, tableFlags );
			if ( lastBeginTable ) {
				ImGui::TableSetupScrollFreeze(1, 0);
				ImGui::TableSetupColumn( "Command", ImGuiTableColumnFlags_WidthFixed, commandColumnWidth );
				for ( int j=0; j < numBindingColumns; ++j ) {
					char colName[16];
					snprintf(colName, sizeof(colName), "binding%d", j);
					ImGui::TableSetupColumn( colName );
				}

				ImGui::TableSetupColumn( "ShowAll", ImGuiTableColumnFlags_WidthFixed, overflowColumnWidth );
			}
		} else if ( isHeading && inTable ) {
			// we've been adding elements to a table (unless lastBeginTable = false) that hasn't
			// been closed with EndTable() yet, but now we're at a heading so the table is done
			if ( lastBeginTable ) {
				ImGui::EndTable();
			}
			inTable = false;
		}

		if ( lastBeginTable ) { // if ImGui::BeginTable() returned false, don't draw its elements
			BindingEntrySelectionState bess = (selectedRow == i) ? selectionState : BESS_NotSelected;
			bess = be.Draw( i, bess );
			if ( bess != BESS_NotSelected ) {
				selectedRow = i;
				selectionState = bess;
			} else if ( selectedRow == i ) {
				// this row was selected, but be.Draw() returned BESS_NotSelected, so unselect it
				selectedRow = -1;
			}
		}
	}
	if ( inTable && lastBeginTable ) {
		// end the last binding table, if any
		ImGui::EndTable();
	}

	if ( ImGui::IsWindowFocused() && IsCancelKeyPressed() ) {
		// pressing Escape unfocuses whatever entry is focused
		selectedRow = -1;
		// FIXME: the following looks dangerous, but for some reason ImGui doesn't
		//        seem to have a function to unfocus..
		// (Escape and gamepad B implicitly unfocus, but gamepad Start doesn't)
		ImGui::GetCurrentContext()->NavId = 0;
	}

	ImGui::PopStyleColor(); // ImGuiCol_TableBorderLight
	ImGui::PopStyleVar(); // ImGuiStyleVar_SelectableTextAlign

	// WantBind or WantClear or WantRebind => show a popup
	if ( selectionState > BESS_Selected ) {
		assert(selectedRow >= 0 && selectedRow < numBindingEntries);
		bindingEntries[selectedRow].HandlePopup( selectionState );
	}
}

static bool show_demo_window = true;

static void myWindow()
{
	ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

	ImGui::SliderFloat( "blaScale", &blaScale, 0.1, 20.0, "%.2f", ImGuiSliderFlags_AlwaysClamp );

	//ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
	ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state

	UpdateWarningOverlay();
	DrawBindingsMenu();

#if 0
	static float f = 0.0f;
	static int counter = 0;
	ImGui::Checkbox("Another Window", &show_another_window);

	ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
	ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

	if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
		counter++;
	ImGui::SameLine();
	ImGui::Text("counter = %d", counter);

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
#endif // 0
	ImGui::End();
}

// Main code
int main(int, char**)
{
    // Setup SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    // From 2.0.18: Enable native IME.
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Setup window
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 4);
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Window* window = SDL_CreateWindow("Dear ImGui SDL2+OpenGL example", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1680, 1050, window_flags);
    if (window == nullptr)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // make it a bit prettier with rounded edges
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 2.0f;
    style.FrameRounding = 3.0f;
    //style.ChildRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 1.0f;
    style.PopupRounding = 2.0f;

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_TitleBg]                = ImVec4(0.28f, 0.36f, 0.48f, 0.88f);
    colors[ImGuiCol_TabHovered]             = ImVec4(0.42f, 0.69f, 1.00f, 0.80f);
    colors[ImGuiCol_TabActive]              = ImVec4(0.24f, 0.51f, 0.83f, 1.00f);

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL2_Init();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return a nullptr. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, nullptr, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != nullptr);

    // Our state

    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    bool done = false;
    while (!done)
    {
        hadKeyDownEvent = false;
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch(event.type)
            {
                case SDL_CONTROLLERBUTTONUP:
                    // DG: special case: I need to know if start is pressed even if ImGui is configured to ignore gamepad input
                    //     (because it's used just like escape) - in Doom3 there's hopefully a better way
                    if ( event.cbutton.button == SDL_CONTROLLER_BUTTON_START ) {
                        gamepadStartPressed = false;
                    }
                    break;
                case SDL_CONTROLLERBUTTONDOWN:
                    if ( event.cbutton.button == SDL_CONTROLLER_BUTTON_START ) {
                        gamepadStartPressed = true;
                    }
                    // fall-through
                case SDL_MOUSEWHEEL:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_KEYDOWN:

                // TODO: controller trigger?
                    hadKeyDownEvent = true; // TODO: for Doom3 the hadKeyDown hack probably also needs to be ported
            }

            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE && event.window.windowID == SDL_GetWindowID(window))
                done = true;
        }
        if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED)
        {
            SDL_Delay(10);
            continue;
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            myWindow();
        }

        // 3. Show another simple window.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // Rendering
        ImGui::Render();
        glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        //glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
