#pragma once

#include "resource.h"

#include <array>

#include <helpers/atl-misc.h>
#include <libPPUI/clipboard.h>
#include <helpers/DarkMode.h>

#include "bookmark_preferences.h"
#include "bookmark_core.h"
#include "bookmark_list_control.h"
#include "bookmark_persistence.h"
#include "bookmark_worker.h"

#include "style_manager_dui.h"
#include "style_manager_cui.h"
#include "guids.h"

using namespace glb;

namespace dlg {

	static const char* COLUMNNAMES[] = { "Time", "Bookmark", "Playlist", "Comment", "Date"};

	static const std::array<uint32_t, N_COLUMNS> defaultColWidths = { 40, 150, 110, 150, 150 };
	static const std::array<bool, N_COLUMNS> defaultColActive = { true, true, false, false, false };

	class CListCtrlMarkDialog : public CDialogImpl<CListCtrlMarkDialog>, private ILOD_BookmarkSource {

	public:

		// cust style manager callback

		void on_style_change() {

			//todo: rev DUI/CUI fonts, etc

			//rev bulky
			const LOGFONT lfh = m_cust_stylemanager->getTitleFont();
			HFONT hOldFontHeader = m_guiList.GetHeaderCtrl().GetFont();
			HFONT hFontHeader = CreateFontIndirect(&lfh);
			m_guiList.SetHeaderFont(hFontHeader);
			//todo: rev already managed (CFont)?
			if (hOldFontHeader != hFontHeader) {
				::DeleteObject(hOldFontHeader);
			}

			const LOGFONT lf = m_cust_stylemanager->getListFont();
			HFONT hOldFont = m_guiList.GetFont();
			HFONT hFont = CreateFontIndirect(&lf);
			m_guiList.SetFont(hFont);
			//todo: rev not managed (CFontHandle)?
			if (hOldFont != hFont) {
				::DeleteObject(hOldFont);
			}

			m_guiList.Invalidate();
		}

		//todo: rev cui
		void applyDark() {
			t_ui_color color = 0;
			if (m_callback->query_color(ui_color_darkmode, color)) {
				m_dark.SetDark(color == 0);
			}
		}

		// DUI constructor

		CListCtrlMarkDialog(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb)
			: m_cfg(cfg.get_ptr()), m_callback(cb), m_cust_stylemanager(new DuiStyleManager(cb)), m_guiList(this, false)
		{

			m_cui = false;

			m_colActive = defaultColActive;
			m_colContent.resize(N_COLUMNS);

			parseConfig(cfg, m_colWidths, m_colActive);

			m_cust_stylemanager->setChangeHandler([&] { this->on_style_change(); });
		}

		// CUI constructor

		CListCtrlMarkDialog(HWND parent, std::array<uint32_t, N_COLUMNS> colWidths, std::array<bool, N_COLUMNS> colActive)
			: m_colWidths(colWidths), m_colActive(colActive), m_guiList(this, true), m_cust_stylemanager(new CuiStyleManager())
		{

			m_cui = true;

			if (!colWidths.size()) {
				m_colActive = defaultColActive;
			}

			m_colContent.resize(N_COLUMNS);
			
			parseConfig(nullptr, m_colWidths, m_colActive);

			m_cust_stylemanager->setChangeHandler([&] { this->on_style_change(); });

			// initiale (create)

			initialize_window(parent);

		}

		// DESTRUCTOR

		~CListCtrlMarkDialog() {

			if (g_primaryGuiList == &m_guiList) {
				g_primaryGuiList = NULL;
			}

			g_guiLists.remove(&m_guiList);

			if (m_cust_stylemanager) {
				delete m_cust_stylemanager;
			}
		}

		enum { IDD = IDD_BOOKMARK_DIALOG };

		BEGIN_MSG_MAP_EX(CListCtrlMarkDialog)
			MSG_WM_INITDIALOG(OnInitDialog)
			MSG_WM_SIZE(OnSize)
			MSG_WM_CONTEXTMENU(OnContextMenu)
			END_MSG_MAP()

		void initialize_window(HWND parent) {

			WIN32_OP(Create(parent) != NULL);
		}

		//see ImplementBumpableElem<TImpl> - ui_element_impl_withpopup<`anonymous-namespace'::dui_tour,ui_element_v2>
		static ui_element_config::ptr g_get_default_configuration() {
			return makeConfig(guid_dui_bmark);
		}

		// Restore Bookmarks

		static void restoreFocusedBookmark() {

			if (g_guiLists.empty() || (g_primaryGuiList && g_primaryGuiList->GetFocusItem() != pfc_infinite)) {
				FB2K_console_print_v("Global Bookmark Restore: No bookmark UI found, falling back to last bookmark");
				restoreBookmark(g_masterList.size() -1);
				return;
			}

			size_t focused;
			if (g_primaryGuiList != NULL) {
				focused = g_primaryGuiList->GetFocusItem();
			}
			else {
				FB2K_console_print_v("Global Bookmark Restore: No primary UI found, falling back to firstborn UI.");
				auto it = g_guiLists.begin();
				focused = (*it)->GetFocusItem();
			}
			restoreBookmark(focused);
		}

		// Restore bookmark by index

		static void restoreBookmark(size_t index) {
			bookmark_worker bmWorker;
			bmWorker.restore(g_masterList, index);
		}

		static void addBookmark() {

			CancelUIListEdits();

			bookmark_worker bmWorker;
			bmWorker.store(g_masterList);

			for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				(*it)->SelectNone();
                (*it)->OnItemsInserted(g_masterList.size() - 1, 1, true);
                (*it)->EnsureItemVisible(g_masterList.size() - 1, false);
			}

			FB2K_console_print_v("Created Bookmark, saving to file now.");
			g_permStore.writeDataFile(g_masterList);
		}

		static void clearBookmarks() {
			CListCtrlMarkDialog::CancelUIListEdits();
			g_masterList.clear();
			for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				(*it)->OnItemsRemoved(pfc::bit_array_true(),g_masterList.size());
			}
			g_permStore.writeDataFile(g_masterList);
		}

		static void CancelUIListEdits() {

			for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				
				if ((*it)->TableEdit_IsActive()) {
					(*it)->TableEdit_Abort(false);
				}
			}
		}

		static bool canStore() {
			return play_control::get()->is_playing();
		}

		static bool canRestore() {
			return (g_primaryGuiList && g_primaryGuiList->GetSingleSel() != ~0)
				|| g_masterList.size();
		}

		static bool canClear() {
			return (g_primaryGuiList && (bool)g_primaryGuiList->GetItemCount())
				|| g_masterList.size();
		}

		//========================UI code===============================

		BOOL OnInitDialog(CWindow, LPARAM) {

			if (g_guiLists.size() > 1) {
				::ShowWindow(GetDlgItem(IDC_STATIC_UI_UNSUPPORTED), SW_SHOW);
				::ShowWindow(GetDlgItem(IDC_BOOKMARKLIST), SW_HIDE);
				return FALSE;
			}

			::ShowWindow(GetDlgItem(IDC_STATIC_UI_UNSUPPORTED), SW_HIDE);
			::ShowWindow(GetDlgItem(IDC_BOOKMARKLIST), SW_SHOW);

			m_guiList.CreateInDialog(*this, IDC_BOOKMARKLIST);

			m_guiList.Initialize(&m_colContent);
			configToUI(false);

			m_dark.AddDialogWithControls(*this);

			g_guiLists.emplace_back(&m_guiList);
			if (g_primaryGuiList == NULL) {
				g_primaryGuiList = &m_guiList;
			}

			on_style_change();
			ShowWindow(SW_SHOW);

			return true; // system should set focus
		}

		void OnSize(UINT, CSize s) {

			CRect recUI;
			CRect recList;

			if (m_guiList.m_hWnd) {
				::GetWindowRect(m_hWnd, &recUI);
				::GetWindowRect(m_guiList.m_hWnd, &recList);
				::MoveWindow(m_guiList.m_hWnd, 0, 0, recUI.Width(), recUI.Height(), 1);
			}
		}

		void OnContextMenu(CWindow wnd, CPoint point) {

			try {

				if (!m_guiList.m_hWnd || (!m_cui && m_callback->is_edit_mode_enabled())) {
					SetMsgHandled(false);
					return;
				}

				// did we get a (-1,-1) point due to context menu key rather than right click?
				// GetContextMenuPoint fixes that, returning a proper point at which the menu should be shown
				point = m_guiList.GetContextMenuPoint(point);

				CMenu menu;
				// WIN32_OP_D() : debug build only return value check
				// Used to check for obscure errors in debug builds, does nothing (ignores errors) in release build
				WIN32_OP_D(menu.CreatePopupMenu());

				CRect headerRct;
				m_guiList.GetHeaderCtrl().GetWindowRect(headerRct);

				if (headerRct.PtInRect(point)) {

					//Header columns
					const int stringlength = 25;
					for (uint32_t i = 0; i < N_COLUMNS; i++) {

						//Need to convert to UI friendly stringformat first:
						wchar_t wideString[stringlength];
						size_t outSize;
						mbstowcs_s(&outSize, wideString, stringlength, COLUMNNAMES[i], stringlength - 1);

						//The + 1 and - 1 below to ensure cmd = 0 remains unused

						auto flags = MF_STRING;
						if (m_colActive[i])
							flags |= MF_CHECKED;
						else
							flags |= MF_UNCHECKED;

						menu.AppendMenu(flags, i + 1, wideString);
					}

					int cmd;
					{
						// status bar command descriptions
						// it's actually a hidden window, needs a parent HWND, where we feed our control's HWND
						CMenuDescriptionMap descriptions(m_hWnd);

						// Set descriptions of all our items
						descriptions.Set(TIME_COL + 1, "Playback timestamp");
						descriptions.Set(DESC_COL + 1, "Custom bookmark description");
						descriptions.Set(PLAYLIST_COL + 1, "Playlist");
						descriptions.Set(ELU_COL + 1, "Comment");
						descriptions.Set(DATE_COL + 1, "Bookmark date");

						cmd = menu.TrackPopupMenuEx(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, descriptions, nullptr);
					}

					if (cmd) {
						size_t colndx = static_cast<size_t>(cmd) - 1;
						m_colActive[colndx] = !m_colActive[colndx]; // toggle column state whose menucommand was triggered
						bool all_disabled = std::find(m_colActive.begin(), m_colActive.end(), true) == m_colActive.end();
						if (all_disabled) {
							//revert
							m_colActive[colndx] = !m_colActive[colndx];
						}
						else {
							configToUI(true);
						}
					}
				}
				else {

					bool bupdatable = true;

					auto selmask = m_guiList.GetSelectionMask();
					auto isel = m_guiList.GetSingleSel();
					size_t icount = m_guiList.GetItemCount();
					size_t csel = m_guiList.GetSelectedCount();

					bool bsinglesel = m_guiList.GetSingleSel() != ~0;
					bool bresetable = (bool)icount && bsinglesel && g_masterList.at(isel).get_time();
					bresetable |= csel > 1;

					//Contextmenu for listbody
					enum { ID_STORE = 1, ID_RESTORE, ID_RESET_TIME, ID_DEL, ID_CLEAR,
						ID_COPY_BOOKMARK, ID_COPY, ID_COPY_PATH, ID_OPEN_FOLDER, ID_SELECTALL, ID_SELECTNONE, ID_INVERTSEL, ID_MAKEPRIME,
						ID_PAUSE_BOOKMARKS, ID_PREF_PAGE, ID_SEL_PROPERTIES
					};
					menu.AppendMenu(MF_STRING | (!CListCtrlMarkDialog::canStore() ? MF_DISABLED | MF_GRAYED : 0), ID_STORE, L"&Add bookmark");
					menu.AppendMenu(MF_STRING | (!bupdatable || !bresetable ? MF_DISABLED | MF_GRAYED : 0), ID_RESET_TIME, L"Reset &time");
					menu.AppendMenu(MF_STRING | (!bsinglesel ? MF_DISABLED | MF_GRAYED : 0), ID_RESTORE, L"&Restore\tENTER");
					menu.AppendMenu(MF_STRING | (!bupdatable || !(bool)csel ? MF_DISABLED | MF_GRAYED : 0), ID_DEL, L"&Delete\tDel");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (!bupdatable || !(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_CLEAR, L"C&lear all");
					menu.AppendMenu(MF_SEPARATOR);
					if (bsinglesel) {
						menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_COPY, L"&Copy");
						menu.AppendMenu(MF_STRING | (!(bool)icount || !m_colActive[1] ? MF_DISABLED | MF_GRAYED : 0), ID_COPY_BOOKMARK, L"Copy &bookmark");
						menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_COPY_PATH, L"Copy &path");
						menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_OPEN_FOLDER, L"&Open containing folder");
						menu.AppendMenu(MF_SEPARATOR);
					}
					// Note: Ctrl+A handled automatically by CListControl, no need for us to catch it
					menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_SELECTALL, L"&Select all\tCtrl+A");
					menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_SELECTNONE, L"Select &none");
					menu.AppendMenu(MF_STRING | (!(bool)csel ? MF_DISABLED | MF_GRAYED : 0), ID_INVERTSEL, L"&Invert selection");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (is_cfg_Bookmarking() ? MF_UNCHECKED : MF_CHECKED), ID_PAUSE_BOOKMARKS, L"&Pause bookmarking");
					menu.AppendMenu(MF_STRING, ID_PREF_PAGE, L"Vital Bookmarks con&figuration...");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (!bsinglesel ? MF_DISABLED | MF_GRAYED : 0), ID_SEL_PROPERTIES, L"Properties\tAlt+ENTER");

					int cmd;
					{
						CMenuDescriptionMap descriptions(m_hWnd);

						descriptions.Set(ID_STORE, "This stores the playback position to a bookmark");
						descriptions.Set(ID_RESTORE, "This restores the playback position from a bookmark");
						descriptions.Set(ID_DEL, "This deletes all selected bookmarks");
						descriptions.Set(ID_CLEAR, "This deletes all  bookmarks");
						descriptions.Set(ID_SELECTALL, "Selects all items");
						descriptions.Set(ID_SELECTNONE, "Deselects all items");
						descriptions.Set(ID_INVERTSEL, "Invert selection");
						descriptions.Set(ID_INVERTSEL, "The primary list's selection determines the bookmark restored by the global restore command.");

						cmd = menu.TrackPopupMenuEx(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, descriptions, nullptr);
					}

					pfc::string8 clip_bookmark;
					GUID guid_ctx = pfc::guid_null;

					switch (cmd) {

					case ID_STORE:
						addBookmark();
						break;
					case ID_RESTORE:
						restoreFocusedBookmark();
						break;
					case  ID_RESET_TIME: {
						auto mask = m_guiList.GetSelectionMask();
						size_t c = m_guiList.GetItemCount();
						auto dbgc = g_masterList.size();
						size_t f = mask.find(true, 0, c);
						for (size_t w = f; w < c; w = mask.find(true, w + 1, c)) {
							g_masterList.at(w).set_time(0.0);
						}

						m_guiList.ReloadItems(mask);
						m_guiList.UpdateItems(mask);

						break;
					}
					case ID_DEL:
						m_guiList.RequestRemoveSelection();
						break;
					case ID_CLEAR:
						clearBookmarks();
						break;
					case ID_COPY_BOOKMARK: {

						if (m_colActive[1]) {
							pfc::string8 coltext;
							for (auto i = 0; i < static_cast<int>(m_guiList.GetColumnCount()); i++) {
								m_guiList.GetColumnText(i, coltext);
								if (coltext.equals(COLUMNNAMES[1])) {
									m_guiList.GetSubItemText(m_guiList.GetSingleSel(), i, clip_bookmark);
									break;
								}
							}
							if (!clip_bookmark.get_length()) {
								break;
							}
						}
						else {
							break;
						}
					}

					[[fallthrough]];
					case ID_COPY_PATH: {

						if (!clip_bookmark.get_length()) {
							clip_bookmark = g_masterList.at(m_guiList.GetSingleSel()).path;
							if (pfc::strcmp_partial(clip_bookmark.get_ptr(), "file://") == 0) {
								clip_bookmark.remove_chars(0, strlen("file://"));
							}
						}

						ClipboardHelper::OpenScope scope;
						scope.Open(core_api::get_main_window(), true);
						ClipboardHelper::SetString(clip_bookmark);
						scope.Close();
						break;
					}
					case ID_SELECTALL:
						m_guiList.SelectAll();
						break;
					case ID_SELECTNONE:
						m_guiList.SelectNone();
						break;
					case ID_INVERTSEL:
					{
						auto selmask = m_guiList.GetSelectionMask();
						m_guiList.SetSelection(
							// Items which we alter - all of them
							pfc::bit_array_true(),
							// Selection values - inverted original selection mask
							pfc::bit_array_not(selmask)
						);
					}
					break;
					case ID_MAKEPRIME:
						g_primaryGuiList = &m_guiList;
						break;

					case ID_PAUSE_BOOKMARKS: {

						if (m_guiList.TableEdit_IsActive()) {
							m_guiList.TableEdit_Abort(true);
						}

						auto res = cfg_status_flag.get_value();
						res  ^= STATUS_PAUSED_FLAG;
						cfg_status_flag.set(res);
						if (g_wnd_bookmark_pref) {
							SendMessage(g_wnd_bookmark_pref, UMSG_PAUSED, NULL, NULL);
						}
						break;
					}
					case ID_PREF_PAGE: {
						if (::IsWindow(g_wnd_bookmark_pref)) {
							::SetFocus(g_wnd_bookmark_pref);
						}
						else {
							static_api_ptr_t<ui_control>()->show_preferences(g_get_prefs_guid());
						}
						break;
					}
					case ID_OPEN_FOLDER: {
						if (pfc::guid_equal(guid_ctx, pfc::guid_null)) {
							menu_helpers::name_to_guid_table menu_table;
							bool bf = menu_table.search("Open containing folder", 22, guid_ctx);
						}
					}
					[[fallthrough]];
					case ID_SEL_PROPERTIES: {
						if (pfc::guid_equal(guid_ctx, pfc::guid_null)) {
							menu_helpers::name_to_guid_table menu_table;
							bool bf = menu_table.search("Properties", 10, guid_ctx);
						}
					}
					[[fallthrough]];
					case ID_COPY: {
						if (pfc::guid_equal(guid_ctx, pfc::guid_null)) {
							menu_helpers::name_to_guid_table menu_table;
							bool bf = menu_table.search("Copy", 4, guid_ctx);
						}

						const char* path = g_masterList.at(m_guiList.GetSingleSel()).path;
						const t_uint32 subsong = g_masterList.at(m_guiList.GetSingleSel()).subsong;
						auto l_metadb = metadb::get();
						metadb_handle_list valid_handles;

						try {
							metadb_handle_ptr ptr;
							l_metadb->handle_create(ptr, make_playable_location(path, subsong));
							valid_handles.add_item(ptr);
						}
						catch (...) {
							break;
						}

						bool rrs = menu_helpers::run_command_context(guid_ctx, pfc::guid_null, valid_handles);
						break;
					}
					}
				}// contextmenu

			}
			catch (std::exception const& e) {
				FB2K_console_print_e("Context menu failure", e); //??
			}
		};

	public:

		ui_element_config::ptr get_configuration(GUID ui_guid) {

			FB2K_console_print_v("get_configuration called.");

			for (int i = 0; i < N_COLUMNS; i++) {
				auto col_ndx = m_colContent[i];
				if (i < static_cast<int>(m_guiList.GetColumnCount())) {
					m_colWidths[col_ndx] = static_cast<int>(m_guiList.GetColumnWidthF(i));
				}
				else {
					//default
					m_colWidths[col_ndx] = defaultColWidths[col_ndx];
				}
			}

			return makeConfig(ui_guid, m_colWidths, m_colActive);
		}

		// host to dlg

		void set_configuration(ui_element_config::ptr config) {
			FB2K_console_print_v("set_configuration called.");
			parseConfig(config, m_colWidths, m_colActive);

			configToUI(false);
		}

		void CUI_gets_config(stream_writer* p_writer, abort_callback& p_abort) const {

			std::array<uint32_t, N_COLUMNS> colWidths = m_colWidths;

			for (int i = 0; i < N_COLUMNS; i++) {
				auto col_ndx = m_colContent[i];
				if (i < static_cast<int>(m_guiList.GetColumnCount())) {
					colWidths[col_ndx] = static_cast<int>(m_guiList.GetColumnWidthF(i));
				}
				else {
					//default
					colWidths[col_ndx] = defaultColWidths[col_ndx];
				}
			}

			stream_writer_formatter<false> writer(*p_writer, p_abort);
			for (int i = 0; i < N_COLUMNS; i++) {
				writer << colWidths[i];
			}
			for (int i = 0; i < N_COLUMNS; i++) {
				writer << m_colActive[i];
			}
		}

	private:

		// read dui config
		static void parseConfig(ui_element_config::ptr cfg, std::array<uint32_t, N_COLUMNS>& widths, std::array<bool, N_COLUMNS>& active) {

			FB2K_console_print_v("Parsing config");

			if (!widths.size()) {
				widths = defaultColWidths;
				active = defaultColActive;
			}
			else {
				//..
			}

			if (!cfg.get_ptr()) {
				//todo: cui
				return;
			}

			try {
				::ui_element_config_parser configParser(cfg);
				//read from config:
				for (int i = 0; i < N_COLUMNS; i++)
					configParser >> widths[i];
				for (int i = 0; i < N_COLUMNS; i++) {
					configParser >> active[i];
				}
			}
			catch (exception_io_data_truncation e) {
				FB2K_console_print_e("Failed to parse configuration", e);
			}
			catch (exception_io_data e) {
				FB2K_console_print_e("Failed to parse configuration", e);
			}
		}

		static ui_element_config::ptr makeConfig(GUID ui_guid, std::array<uint32_t, N_COLUMNS> widths = defaultColWidths, const std::array<bool, N_COLUMNS> active = defaultColActive) {
			if (sizeof(widths) / sizeof(uint32_t) != N_COLUMNS)
				return makeConfig(ui_guid);

			FB2K_console_print_v("Making config from ", widths[0], " and ", widths[1]);

			ui_element_config_builder out;
			for (int i = 0; i < N_COLUMNS; i++)
				out << widths[i];
			for (int i = 0; i < N_COLUMNS; i++)
				out << active[i];
			return out.finish(ui_guid);
		}

		void configToUI(bool breload) {

			FB2K_console_print_v("Applying config to UI: ", m_colWidths[0], " and ", m_colWidths[1]);

			auto DPI = m_guiList.GetDPI();

			if (m_guiList.GetHeaderCtrl() != NULL && m_guiList.GetHeaderCtrl().GetItemCount()) {
				m_guiList.ResetColumns(false);
			}

			auto fit = std::find(m_colActive.begin(), m_colActive.end(), true);
			if (fit == m_colActive.end()) {
				m_colActive[0] = true;
				m_colActive[1] = true;
			}

			size_t ndx_tail = N_COLUMNS - 1;
			for (int i = 0; i < N_COLUMNS; i++) {

				if (cfg_verbose) {
					FB2K_console_print_v("Config to UI: i is ", i, "; name: ", COLUMNNAMES[i], ", active: ", m_colActive[i], ", width: ", m_colWidths[i]);
				}

				auto ndx_cont = !m_guiList.IsHeaderEnabled() ? 0 : m_guiList.GetColumnCount();
				if (m_colActive[i]) {
					//use defaults instead of zero
					size_t width = (m_colWidths[i] != 0 && m_colWidths[i] != pfc_infinite) ? m_colWidths[i] : defaultColWidths[i];
					width = pfc::min_t<size_t>(width, 1000);
					m_colContent[ndx_cont] = i;
					m_guiList.AddColumn(COLUMNNAMES[i], MulDiv(static_cast<int>(width), DPI.cx, 96), LVCFMT_LEFT, false);
				}
				else {
					//move to tail
					m_colContent[ndx_tail--] = i;
				}
			}
			if (breload) {
				m_guiList.ReloadItems(bit_array_true());
				m_guiList.Invalidate(1);
			}
		}

	protected:

		const ui_element_instance_callback::ptr m_callback;
		const ui_element_config::ptr m_cfg;

		StyleManager* m_cust_stylemanager = nullptr;
		CListControlBookmark m_guiList;

	private:

		fb2k::CDarkModeHooks m_dark;

		bool m_cui = false;

		std::array<uint32_t, N_COLUMNS> m_colWidths = {0};

		std::array<bool, N_COLUMNS> m_colActive;
		pfc::array_t<size_t> m_colContent;

		friend class CListControlBookmark;
	};
}
