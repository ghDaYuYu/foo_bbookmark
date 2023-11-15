#include "stdafx.h"
#include <helpers/BumpableElem.h>
#include "guids.h"
#include "bookmark_list_dialog.h"

namespace {

	// DUI Element class

	class dui_bmark : public CListCtrlMarkDialog, public ui_element_instance {

	public:

		dui_bmark(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) :
			CListCtrlMarkDialog(cfg, cb) {} // dui interface

		static void g_get_name(pfc::string_base& out) { out = COMPONENT_NAME_HC; }
		static GUID g_get_guid() { return guid_dui_bmark; }
		static const char* g_get_description() { return COMPONENT_DESC; }
		static GUID g_get_subclass() { return ui_element_subclass_utility; }

		HWND get_wnd() { return m_hWnd; }

		// dlg to host
		ui_element_config::ptr get_configuration() {
			ui_element_config_builder out;
			get_uicfg(&out, fb2k::noAbort);
			return out.finish(get_guid());
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
	// S T A T I C   D U I - E L E M E N T

	class dui_mark_impl : public ui_element_impl_withpopup<dui_bmark> {};
	static service_factory_single_t<dui_mark_impl> dui_bmark_Element;

}