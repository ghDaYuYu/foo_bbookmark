#include "stdafx.h"

static const GUID guid_mainmenu_basic_bookmark_group_id = { 0xf20f48a9, 0x82a7, 0x48b8, { 0xb5, 0x5c, 0x69, 0x1a, 0xba, 0x97, 0x34, 0x6 } };
static const GUID guid_storeBookmark = { 0xa85898f, 0x1fd7, 0x4d1b, { 0xb3, 0x18, 0x59, 0x53, 0xd3, 0xe3, 0x43, 0x1b } };
static const GUID guid_restoreBookmark = { 0x533788da, 0xc485, 0x49e7, { 0x99, 0x27, 0x6, 0xf3, 0x89, 0x1a, 0x9a, 0xe0 } };
static const GUID guid_clearBookmarks = { 0xe3d82036, 0xef58, 0x433a, { 0x8b, 0x80, 0x42, 0x18, 0xcf, 0x80, 0x3c, 0xe3 } };



static mainmenu_group_popup_factory g_mainmenu_group(guid_mainmenu_basic_bookmark_group_id, mainmenu_groups::playback, mainmenu_commands::sort_priority_dontcare, "Basic Bookmark");


void bbookmarkHook_store(); 	//bookmark_dialog.cpp
void bbookmarkHook_restore(); 	//bookmark_dialog.cpp
void bbookmarkHook_clear(); 	//bookmark_dialog.cpp

class mainmenu_commands_basic_bookmark : public mainmenu_commands {
public:
	enum {
		cmd_store = 0,
		cmd_restore,
		cmd_clearBookmarks,
		cmd_total
	};
	t_uint32 get_command_count() {
		return cmd_total;
	}
	GUID get_command(t_uint32 p_index) {

		switch (p_index) {
		case cmd_store: return guid_storeBookmark;
		case cmd_restore: return guid_restoreBookmark;
		case cmd_clearBookmarks: return guid_clearBookmarks;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	void get_name(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case   cmd_store: p_out = "Add Bookmark; break;
		case cmd_restore: p_out = "Restore Bookmark"; break;
		case cmd_clearBookmarks: p_out = "Clear Bookmarks"; break;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case   cmd_store: p_out = "Stores the playback position to a bookmark"; return true;
		case cmd_restore: p_out = "Restores the playback position from the bookmark selected by in the first element to be instantiated."; return true;
		case cmd_clearBookmarks: p_out = "Removes all bookmarks"; return true;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
	GUID get_parent() {
		return guid_mainmenu_basic_bookmark_group_id;
	}
	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
		switch (p_index) {
		case   cmd_store:
			bbookmarkHook_store();
			break;
		case cmd_restore:
			bbookmarkHook_restore();
			break;
		case cmd_clearBookmarks:
			bbookmarkHook_clear();
			break;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}
};

static mainmenu_commands_factory_t<mainmenu_commands_basic_bookmark> g_mainmenu_commands_basic_bookmark_factory;