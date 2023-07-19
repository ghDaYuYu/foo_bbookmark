#include "stdafx.h"

#include "bookmark_list_dialog.h"

static const GUID guid_bookmark_ui = { 0xadaa758c, 0xc2c2, 0x4f3b, { 0x83, 0x12, 0x39, 0x86, 0x63, 0xbb, 0x67, 0x36 } };

using namespace dlg;

namespace {

	// DUI Element class

	class dui_bmark : public CListCtrlMarkDialog, public ui_element_instance {

	public:

		dui_bmark(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) :
			CListCtrlMarkDialog(parent, cfg, cb) {} // dui interface

		// todo: g_get_guid
		GUID get_guid() { return guid_bookmark_ui; }
		GUID get_subclass() { return ui_element_subclass_utility; }
		void get_name(pfc::string_base& out) { out = COMPONENT_NAME_HC; }
		const char* get_description() { return "Provides playback bookmark functionality."; }

		HWND get_wnd() { return m_hWnd; }

		// dlg to host
		ui_element_config::ptr get_configuration() {
			return CListCtrlMarkDialog::get_configuration(get_guid());
		}

		// host to dlg
		void set_configuration(ui_element_config::ptr config) {
			CListCtrlMarkDialog::set_configuration(config);
		}

		void notify(const GUID& p_what, t_size p_param1, const void* p_param2, t_size p_param2size) /*override*/ {

			if (p_what == ui_element_notify_colors_changed) {
				applyDark();
			}

			if (p_what == ui_element_notify_colors_changed) {

				m_cust_stylemanager->onChange();
				Invalidate();
			}
			else if (p_what == ui_element_notify_font_changed) {
				m_cust_stylemanager->onChange();
				Invalidate();
			}
			else if (p_what == ui_element_notify_visibility_changed) {
				//..
			}
			else {
				//..
			}
		}
	};

	class UiElement : public ui_element {

	public:

		// instantiate

		ui_element_instance::ptr instantiate(HWND parent, ui_element_config::ptr cfg, ui_element_instance_callback::ptr callback) final {

			PFC_ASSERT(cfg->get_guid() == get_guid());

			service_nnptr_t<dui_bmark> item =
				new service_impl_t<dui_bmark>(parent, cfg, callback);

			return item;
		}

		// todo: g_get_guid
		GUID get_guid() final { return guid_bookmark_ui; }
		GUID get_subclass() final { return ui_element_subclass_utility; }
		void get_name(pfc::string_base& out) final { out = COMPONENT_NAME_HC; }

		bool get_description(pfc::string_base& out) final {
			out = "Provides track playback bookmark functionality.";
			return true;
		}

		ui_element_config::ptr get_default_configuration() final {

			return ui_element_config::g_create_empty(guid_bookmark_ui);
		}

		ui_element_children_enumerator_ptr enumerate_children(
			ui_element_config::ptr /*cfg*/) final {
			return nullptr;
		}
	};

	static service_factory_single_t<UiElement> uiElement;

}