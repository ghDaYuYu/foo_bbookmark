#include "stdafx.h"
#include <optional>

#include "bookmark_core.h"
#include "bookmark_list_dialog.h"
#include "style_manager.h"

static const GUID guid_cui_bmark = { 0x70b26ed8, 0x710, 0x4d36, { 0xb8, 0xfe, 0xf7, 0xcf, 0x83, 0x41, 0x7, 0x62 } };

using namespace dlg;

namespace {

	// CUI element class

	class cui_bmark : public ui_extension::window {

		ui_extension::window_host_ptr m_host;
		std::optional<CListCtrlMarkDialog> window;

	public:

		cui_bmark() = default;

		void get_category(pfc::string_base& out) const final { out = "Panels"; }

		const GUID& get_extension_guid() const final { return guid_cui_bmark; }
		HWND get_wnd() const override final { return window->get_wnd(); }
		void get_name(pfc::string_base& out) const final { out = COMPONENT_NAME_HC; }
		unsigned get_type() const final { return ui_extension::type_panel; }

		bool is_available(const ui_extension::window_host_ptr& p_host) const final {
			return !m_host.is_valid() || p_host->get_host_guid() != m_host->get_host_guid();
		}

		const bool get_is_single_instance() const override final { return true; }

		// create or transfer

		HWND create_or_transfer_window(HWND wnd_parent,
			const ui_extension::window_host_ptr& p_host,
			const ui_helpers::window_position_t& p_position) final override {

			if (!window || !window->get_wnd()) {

				try {

					// create dlg

					window.emplace(wnd_parent, hosted_colWidths, hosted_colActive);

				}
				catch (std::exception& e) {
					FB2K_console_formatter()
						<< COMPONENT_NAME << " panel failed to initialize: " << e.what();
					return nullptr;
				}

				m_host = p_host;
				ShowWindow(window->get_wnd(), SW_HIDE);
				SetWindowPos(window->get_wnd(), nullptr, p_position.x, p_position.y, p_position.cx,
					p_position.cy, SWP_NOZORDER);
			}
			else {

				// transfer dlg

				ShowWindow(window->get_wnd(), SW_HIDE);
				SetParent(window->get_wnd(), wnd_parent);
				SetWindowPos(window->get_wnd(), nullptr, p_position.x, p_position.y, p_position.cx,
					p_position.cy, SWP_NOZORDER);

				m_host->relinquish_ownership(window->get_wnd());
				m_host = p_host;
			}

			return window->get_wnd();
		}

		// set_config

		void set_config(stream_reader* p_reader, t_size p_size, abort_callback& p_abort) override {
			
			//window is not created yet
			stream_reader_formatter<false> reader(*p_reader, p_abort);

			for (int i = 0; i < N_COLUMNS; i++) {
				reader >> (t_uint32)hosted_colWidths[i];
			}
			for (int i = 0; i < N_COLUMNS; i++) {
				reader >> (bool)hosted_colActive[i];
			}
		};

		// get_config

		void get_config(stream_writer* p_writer, abort_callback& p_abort) const override {

			window->CUI_gets_config(p_writer, p_abort);

		};

		void destroy_window() final {

			//..

		}

	private:

		std::array<uint32_t, N_COLUMNS> hosted_colWidths;
		std::array<bool, N_COLUMNS> hosted_colActive;

	};

	static service_factory_single_t<cui_bmark> cui_bmark_instance;

}