#include "stdafx.h"
#include "SDK\contextmenu.h"

#include "bookmark_preferences.h"
#include "context_menu.h"

//ref. to bookmark_dialog.cpp
void bbookmarkHook_store();
void bbookmarkHook_restore();
void bbookmarkHook_clear();

bool bbookmarkHook_canStore();
bool bbookmarkHook_canRestore();
bool bbookmarkHook_canClear();

unsigned contextmenu_item_foo_vb::get_num_items()
{
	return 1;
}

contextmenu_item_node_root* contextmenu_item_foo_vb::instantiate_item(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller)
{
	return new contextmenu_item_node_root_popup_vb(p_index);
}

GUID contextmenu_item_foo_vb::get_item_guid(unsigned p_index)
{
	return item_guid[p_index];
}

void contextmenu_item_foo_vb::get_item_name(unsigned p_index, pfc::string_base& p_out)
{
	p_out = item_name[p_index];
}

void contextmenu_item_foo_vb::get_item_default_path(unsigned p_index, pfc::string_base& p_out)
{
	p_out = "";
}

bool contextmenu_item_foo_vb::get_item_description(unsigned p_index, pfc::string_base& p_out)
{
	p_out = item_name[p_index];
	return true;
}

contextmenu_item::t_enabled_state contextmenu_item_foo_vb::get_enabled_state(unsigned p_index)
{
	return contextmenu_item::DEFAULT_ON;
}

void contextmenu_item_foo_vb::item_execute_simple(unsigned p_index, const GUID& p_node, metadb_handle_list_cref p_data, const GUID& p_caller)
{
	//called from toolbar
	if (p_node == pfc::guid_null)
		return;

	//property page
	if (p_node == guid_ctx_menu_node_properties) {
		static_api_ptr_t<ui_control> uc;
		uc->show_preferences(g_get_prefs_guid());
		return;
	}

	if (p_node == guid_ctx_menu_node_add_bookmark) {

		if (p_data.get_count() != 1) {
			return;
		}

		metadb_handle_ptr mhp;
		auto np = playback_control_v3::get()->get_now_playing(mhp);
		if (p_data.get_item(0) != mhp) {
			return;
		}

		//add bookmark
		if (bbookmarkHook_canStore())
			bbookmarkHook_store();
		return;
	}
}

double contextmenu_item_foo_vb::get_sort_priority()
{
	return 0;
}
GUID contextmenu_item_foo_vb::get_parent()
{
	return contextmenu_groups::root;
}

void contextmenu_item_foo_vb::g_context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller)
{
	if (!p_data.get_count())
	{
		popup_message::g_show("No input items.\n", COMPONENT_NAME_HC);
		return;
	}
}

void contextmenu_item_foo_vb::g_get_item_name(unsigned p_index, pfc::string_base& p_out)
{
	p_out = item_name[p_index];
}

const char* const contextmenu_item_foo_vb::item_name[] = { COMPONENT_NAME_HC };

// {2DE982A1-5D33-4D36-BED1-9900932C9D80}
const GUID contextmenu_item_foo_vb::item_guid[] =
{ 0x2de982a1, 0x5d33, 0x4d36, { 0xbe, 0xd1, 0x99, 0x0, 0x93, 0x2c, 0x9d, 0x80 } };

pfc::string8 contextmenu_item_foo_vb::fb2k_path("");


//Root Popup
contextmenu_item_node_root_popup_vb::contextmenu_item_node_root_popup_vb(unsigned p_index) : m_index(p_index)
{ }

bool contextmenu_item_node_root_popup_vb::get_display_data(pfc::string_base& p_out, unsigned& p_displayflags, metadb_handle_list_cref p_data, const GUID& p_caller)
{
	if (p_data.get_count() != 1) {
		return false;
	}

	metadb_handle_ptr mhp;
	auto np = playback_control_v3::get()->get_now_playing(mhp);
	if (p_data.get_item(0) != mhp) {
		return false;
	}

	if (!(pfc::guid_equal(p_caller, contextmenu_item::caller_active_playlist_selection)
		|| pfc::guid_equal(p_caller, contextmenu_item::caller_now_playing))) {
		return false;
	}


	contextmenu_item_foo_vb::g_get_item_name(m_index, p_out);
	return true;
}

t_size contextmenu_item_node_root_popup_vb::get_children_count()
{
	return 2 + 1; // add bookmark + separator + preference page
}

contextmenu_item_node* contextmenu_item_node_root_popup_vb::get_child(t_size p_index)
{
	auto separator_pos = get_children_count() - 2;//- 1 separator - 1 preference page

		if (p_index == separator_pos) {
			return new contextmenu_item_node_separator;
		}
		else {
			if (p_index == 0) {
				return new contextmenu_item_node_add_vb();
			}
			else {
				return new contextmenu_item_node_prop_vb();
			}
		}
}

GUID contextmenu_item_node_root_popup_vb::get_guid()
{
	return pfc::guid_null;
}

bool contextmenu_item_node_root_popup_vb::is_mappable_shortcut()
{
	return false;
}

//Properties
contextmenu_item_node_prop_vb::contextmenu_item_node_prop_vb()
{ }

bool contextmenu_item_node_prop_vb::get_display_data(pfc::string_base& p_out, unsigned& p_displayflags, metadb_handle_list_cref p_data, const GUID& p_caller)
{
	p_displayflags = 0;
	p_out = "Vital Bookmarks preferences...";
	return true;
}

bool contextmenu_item_node_prop_vb::get_description(pfc::string_base& p_out)
{
	p_out = "Vital Bookmarks configuration";
	return true;
}

void contextmenu_item_node_prop_vb::execute(metadb_handle_list_cref p_data, const GUID& p_caller)
{
	static_api_ptr_t<ui_control> uc;
	uc->show_preferences(g_get_prefs_guid());
}

GUID contextmenu_item_node_prop_vb::get_guid()
{
	return guid_ctx_menu_node_properties;
}

bool contextmenu_item_node_prop_vb::is_mappable_shortcut()
{
	return true;
}

//Add bookmark
contextmenu_item_node_add_vb::contextmenu_item_node_add_vb()
{ }

bool contextmenu_item_node_add_vb::get_display_data(pfc::string_base& p_out, unsigned& p_displayflags, metadb_handle_list_cref p_data, const GUID& p_caller)
{
	p_displayflags = 0;
	p_out = "Add bookmark";
	return true;
}

bool contextmenu_item_node_add_vb::get_description(pfc::string_base& p_out)
{
	p_out = "Add bookmark";
	return true;
}

void contextmenu_item_node_add_vb::execute(metadb_handle_list_cref p_data, const GUID& p_caller)
{
	if (bbookmarkHook_canStore())
		bbookmarkHook_store();
}

GUID contextmenu_item_node_add_vb::get_guid()
{
	return guid_ctx_menu_node_add_bookmark;
}

bool contextmenu_item_node_add_vb::is_mappable_shortcut()
{
	return true;
}
//factory
static contextmenu_item_factory_t<contextmenu_item_foo_vb> g_contextmenu_item_foo_vb_factory;
