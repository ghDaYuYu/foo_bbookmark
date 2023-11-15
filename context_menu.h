#pragma once
class contextmenu_item_foo_vb : public contextmenu_item_v2
{
public:
	virtual unsigned get_num_items();
	virtual contextmenu_item_node_root* instantiate_item(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller);
	virtual GUID get_item_guid(unsigned p_index);
	virtual void get_item_name(unsigned p_index, pfc::string_base& p_out);
	virtual void get_item_default_path(unsigned p_index, pfc::string_base& p_out);
	virtual bool get_item_description(unsigned p_index, pfc::string_base& p_out);
	virtual t_enabled_state get_enabled_state(unsigned p_index);
	virtual void item_execute_simple(unsigned p_index, const GUID& p_node, metadb_handle_list_cref p_data, const GUID& p_caller);
	virtual double get_sort_priority();
	virtual GUID get_parent();

	static void g_context_command(unsigned p_index, metadb_handle_list_cref p_data, const GUID& p_caller);
	static void g_get_item_name(unsigned p_index, pfc::string_base& p_out);
private:
	static const char* const item_name[];
	static const GUID item_guid[];
	static pfc::string8 fb2k_path;
};

class contextmenu_item_node_root_popup_vb : public contextmenu_item_node_root_popup
{
private:
	unsigned m_index;
public:
	contextmenu_item_node_root_popup_vb(unsigned p_index);
	bool get_display_data(pfc::string_base& p_out, unsigned& p_displayflags, metadb_handle_list_cref p_data, const GUID& p_caller);
	t_size get_children_count();
	contextmenu_item_node* get_child(t_size p_index);
	GUID get_guid();
	bool is_mappable_shortcut();
};


// {ACEA7509-C27A-46EB-900E-995325181B0B}
static const GUID guid_ctx_menu_node_properties =
{ 0xacea7509, 0xc27a, 0x46eb, { 0x90, 0xe, 0x99, 0x53, 0x25, 0x18, 0x1b, 0xb } };

// {D445A8DB-DFB6-442D-A3C8-7274043C922F}
static const GUID guid_ctx_menu_node_add_bookmark =
{ 0xd445a8db, 0xdfb6, 0x442d, { 0xa3, 0xc8, 0x72, 0x74, 0x4, 0x3c, 0x92, 0x2f } };


class contextmenu_item_node_prop_vb : public contextmenu_item_node_leaf
{
public:
	contextmenu_item_node_prop_vb();
	bool get_display_data(pfc::string_base& p_out, unsigned& p_displayflags, metadb_handle_list_cref p_data, const GUID& p_caller);
	void execute(metadb_handle_list_cref p_data, const GUID& p_caller);
	bool get_description(pfc::string_base& p_out);
	GUID get_guid();
	bool is_mappable_shortcut();
};

class contextmenu_item_node_add_vb : public contextmenu_item_node_leaf
{
public:
	contextmenu_item_node_add_vb();
	bool get_display_data(pfc::string_base& p_out, unsigned& p_displayflags, metadb_handle_list_cref p_data, const GUID& p_caller);
	void execute(metadb_handle_list_cref p_data, const GUID& p_caller);
	bool get_description(pfc::string_base& p_out);
	GUID get_guid();
	bool is_mappable_shortcut();
};
