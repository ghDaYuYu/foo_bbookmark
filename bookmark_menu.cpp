#include "stdafx.h"

static const GUID guid_vbookmark_main_menu_group_id = { 0x4da5c373, 0x8c39, 0x49bd, { 0xaf, 0xe4, 0xe3, 0x4c, 0xb0, 0xd9, 0x43, 0x67 } };

static const GUID guid_storeBookmark = { 0x5ae6fa28, 0xe10, 0x4970, { 0xb3, 0x9, 0x8d, 0x37, 0xf0, 0x4a, 0xba, 0x4b } };
static const GUID guid_restoreBookmark = { 0xc23afd1a, 0xf7bd, 0x4b8f, { 0xa0, 0xff, 0xd, 0xf2, 0x27, 0x1e, 0xe7, 0x48 } };
static const GUID guid_clearBookmarks = { 0x2e65ef5a, 0x8620, 0x4cee, { 0xaf, 0x8f, 0xac, 0xd0, 0x6, 0x97, 0x7b, 0xa9 } };

static mainmenu_group_popup_factory g_mainmenu_group(guid_vbookmark_main_menu_group_id, mainmenu_groups::playback, mainmenu_commands::sort_priority_dontcare, COMPONENT_NAME_HC);

//ref. to bookmark_dialog.cpp
void bbookmarkHook_store();
void bbookmarkHook_restore();
void bbookmarkHook_clear();

bool bbookmarkHook_canStore();
bool bbookmarkHook_canRestore();
bool bbookmarkHook_canClear();

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

	GUID get_parent() {
		return guid_vbookmark_main_menu_group_id;
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
		case cmd_store: p_out = "Add Bookmark"; break;
		case cmd_restore: p_out = "Restore Bookmark"; break;
		case cmd_clearBookmarks: p_out = "Clear Bookmarks"; break;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}

	bool get_description(t_uint32 p_index, pfc::string_base & p_out) {
		switch (p_index) {
		case cmd_store: p_out = "Stores the playback position to a bookmark"; return true;
		case cmd_restore: p_out = "Restores the playback position from the bookmark selected by in the first element to be instantiated."; return true;
		case cmd_clearBookmarks: p_out = "Removes all bookmarks"; return true;
		default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}

	bool get_display(t_uint32 p_index, pfc::string_base& p_text, t_uint32& p_flags) {
		p_flags = 0;
		switch (p_index) {
		case cmd_store:
			p_flags = !bbookmarkHook_canStore();
			break;
		case cmd_restore:
			p_flags = !bbookmarkHook_canRestore();
			break;
		case cmd_clearBookmarks:
			p_flags = !bbookmarkHook_canClear();
			break;
		}
		get_name(p_index, p_text);
		return true;
	}

	void execute(t_uint32 p_index, service_ptr_t<service_base> p_callback) {
		switch (p_index) {
		case cmd_store:
			if (bbookmarkHook_canStore())
				bbookmarkHook_store();
			break;
		case cmd_restore:
			if (bbookmarkHook_canRestore())
				bbookmarkHook_restore();
			break;
		case cmd_clearBookmarks:
			if (bbookmarkHook_canClear())
				bbookmarkHook_clear();
			break;
		default: uBugCheck();
		}
	}
};

static mainmenu_commands_factory_t<mainmenu_commands_basic_bookmark> g_mainmenu_commands_basic_bookmark_factory;
