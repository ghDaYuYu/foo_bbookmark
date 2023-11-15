#include "stdafx.h"
#include <optional>

#include "bookmark_core.h"
#include "guids.h"
#include "bookmark_list_dialog.h"

namespace {

	// CUI element class

	class cui_bmark : public ui_extension::window {

		ui_extension::window_host_ptr m_host;
		std::optional<CListCtrlMarkDialog> window;

	public:

		cui_bmark() = default;

		const GUID& get_extension_guid() const final { return guid_cui_bmark; }
		void get_name(pfc::string_base& out) const final { out = COMPONENT_NAME_HC; }
		void get_category(pfc::string_base& out) const final { out = "Panels"; }
		unsigned get_type() const final { return ui_extension::type_panel; }

		HWND get_wnd() const override final { return window->get_wnd(); }
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

					window.emplace(wnd_parent, m_sorted_dir, m_hosted_colWidths, m_hosted_colActive);

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
			dlg::CListCtrlMarkDialog::set_cui_config(p_reader, p_size, p_abort, m_sorted_dir, m_hosted_colWidths, m_hosted_colActive);

		};

		// get_config

		void get_config(stream_writer* p_writer, abort_callback& p_abort) const override {

			stream_writer_formatter<false> writer(*p_writer, p_abort);
			window->get_uicfg(&writer, p_abort);

		};

		void destroy_window() final {

			//container_uie_window_v3_t::destroy_windows
			window.value().DestroyWindow();
			window.reset();
			m_host.release();
		}

	private:

		std::array<uint32_t, N_COLUMNS> m_hosted_colWidths;
		std::array<bool, N_COLUMNS> m_hosted_colActive;
		bool m_sorted_dir;
	};

	// S T A T I C   C U I - E L E M E N T

	static service_factory_single_t<cui_bmark> cui_bmark_instance;

}