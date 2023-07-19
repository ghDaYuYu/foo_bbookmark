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

using namespace glb;

namespace dlg {

	static const char* COLUMNNAMES[] = { "Time", "Bookmark", "Playlist", "Comment", "Date"};

	static const std::array<uint32_t, N_COLUMNS> defaultColWidths = { 40, 150, 110, 150, 150 };
	static const std::array<bool, N_COLUMNS> defaultColActive = { true, true, false, false, false };

	class CListCtrlMarkDialog : public CDialogImpl<CListCtrlMarkDialog>, private ILOD_BookmarkSource {

	public:
		//---Dialog Setup---
		CListControlBookmarkDialog(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb), m_guiList(this) {
			m_colActive = defaultColActive;
            m_colContent.resize(N_COLUMNS);
            parseConfig(cfg, m_colWidths, m_colActive);
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

		//not overriding - serves cui_bmark
		HWND get_wnd() const { return m_hWnd; }

		// Restore Bookmarks

		static void restoreFocusedBookmark() {

			if (g_guiLists.empty()) {	//fall back to 0 if there is no list, and hence no selected bookmark
				FB2K_console_print_v("Global Bookmark Restore: No bookmark UI found, falling back to first bookmark");
				restoreBookmark(0);
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
			bookmark_worker bmWorker;
			bmWorker.store(g_masterList);

			for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				(*it)->OnItemsInserted(g_masterList.size() - 1, 1, true);
			}

			FB2K_console_print_v("Created Bookmark, saving to file now.");
			g_permStore.writeDataFileJSON(g_masterList);

		}

		static void clearBookmarks() {
			size_t oldCount = g_masterList.size();
			g_masterList.clear();
			for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				(*it)->OnItemsRemoved(pfc::bit_array_true(), oldCount);
			}
			g_permStore.writeDataFileJSON(g_masterList);
		}

		static bool canStore() {

			return play_control::get()->is_playing();
		}

		static bool canRestore() {
			return g_primaryGuiList && g_primaryGuiList->GetSingleSel() != ~0;
		}

		static bool canClear() {
			return g_primaryGuiList && (bool)g_primaryGuiList->GetItemCount();
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

			m_guiList.Initialize(m_hWnd, &m_colContent);


			m_dark.AddDialogWithControls(*this);

			g_guiLists.emplace_back(&m_guiList);
			if (g_primaryGuiList == NULL) {
				g_primaryGuiList = &m_guiList;
			}

			HWND hwndBookmarkList = GetDlgItem(IDC_BOOKMARKLIST);
			CListViewCtrl wndList(hwndBookmarkList);
			wndList.SetParent(m_hWnd);

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
			
			return;
		}

		//===========Overrides for CListControlBookmark================
		size_t listGetItemCount(ctx_t ctx) override {
			PFC_ASSERT(ctx == &m_list); // ctx is a pointer to the object calling us
			return g_masterList.size();
		}
		pfc::string8 listGetSubItemText(ctx_t, size_t item, size_t subItem) override {
			auto & rec = g_masterList[item];
			auto subItemContent = m_colContent[subItem];
			switch (subItemContent) {
			case TIME_COL:
			{
				std::ostringstream conv;
				int hours = (int)rec.m_time / 3600;
				int minutes = (int)std::fmod(rec.m_time, 3600) / 60;
				int seconds = (int)std::fmod(rec.m_time, 60);
				if (hours != 0)
					conv << hours << ":";
				conv << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
				return conv.str().c_str();
			}
			case DESC_COL:
				return rec.m_desc.c_str();
			case PLAYLIST_COL:
				return rec.m_playlist.c_str();
			default:
				return "";
			}

		}
		bool listCanReorderItems(ctx_t) override { return true; }
		bool listReorderItems(ctx_t, const size_t* order, size_t count) override {
			PFC_ASSERT(count == g_masterList.size());
			pfc::reorder_t(g_masterList, order, count);
			g_permStore.writeDataFile(g_masterList);
			return true;
		}
		bool listRemoveItems(ctx_t, pfc::bit_array const & mask) override {
			size_t oldCount = g_masterList.size();

			pfc::remove_mask_t(g_masterList, mask); //remove from global list

			//Update all guiLists
			for (std::list<CListControlBookmark *>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				if ((*it) != &m_guiList) {
					(*it)->OnItemsRemoved(mask, oldCount);
				}
			}

			g_permStore.writeDataFile(g_masterList);	//Write to file
			return true;
		}
		void listItemAction(ctx_t, size_t item) override { restoreBookmark(item); }

		void listSubItemClicked(ctx_t, size_t item, size_t subItem) override { return; }
		void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char * val) override { return; }	//We don't want to allow edits		
		bool listIsColumnEditable(ctx_t, size_t subItem) override { return false; }

		void OnContextMenu(CWindow wnd, CPoint point) {

			try {
				//todo: rev
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
							configToUI();
						}
					}

				}
				else {

					bool bupdatable = (!playback_control_v3::get()->is_playing() || playback_control_v3::get()->is_paused()) || !is_cfg_Bookmarking() || !cfg_autosave_newtrack.get();

					auto selmask = m_guiList.GetSelectionMask();
					auto isel = m_guiList.GetSingleSel();

					size_t csel = m_guiList.GetSelectedCount();
					bool blist_empty = !(bool)m_guiList.GetItemCount();
					bool bsinglesel = m_guiList.GetSingleSel() != ~0;
					bool bresetable = false;
					
					if (bsinglesel) {
						bresetable = g_masterList.at(isel).time != 0;
					}

					//Contextmenu for listbody
					enum { ID_STORE = 1, ID_RESTORE, ID_RESET_TIME, ID_DEL, ID_CLEAR, ID_COPY, ID_SELECTALL, ID_SELECTNONE, ID_INVERTSEL, ID_MAKEPRIME, ID_PAUSE_BOOKMARKS, ID_PREF_PAGE };
					menu.AppendMenu(MF_STRING | (!CListCtrlMarkDialog::canStore() ? MF_DISABLED | MF_GRAYED : 0), ID_STORE, L"&Add Bookmark");
					menu.AppendMenu(MF_STRING | (!bupdatable || !bresetable ? MF_DISABLED | MF_GRAYED : 0), ID_RESET_TIME, L"Reset &time");
					menu.AppendMenu(MF_STRING | (!bsinglesel ? MF_DISABLED | MF_GRAYED : 0), ID_RESTORE, L"&Restore\tENTER");
					menu.AppendMenu(MF_STRING | (!bupdatable || !(bool)csel ? MF_DISABLED | MF_GRAYED : 0), ID_DEL, L"&Delete\tDel");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (!bupdatable || blist_empty ? MF_DISABLED | MF_GRAYED : 0), ID_CLEAR, L"C&lear All");
					menu.AppendMenu(MF_SEPARATOR);
					if (bsinglesel) {
						menu.AppendMenu(MF_STRING, ID_COPY, L"&Copy");
						menu.AppendMenu(MF_SEPARATOR);
					}
					// Note: Ctrl+A handled automatically by CListControl, no need for us to catch it
					menu.AppendMenu(MF_STRING | (blist_empty ? MF_DISABLED | MF_GRAYED : 0), ID_SELECTALL, L"&Select all\tCtrl+A");
					menu.AppendMenu(MF_STRING | (blist_empty ? MF_DISABLED | MF_GRAYED : 0), ID_SELECTNONE, L"Select &none");
					menu.AppendMenu(MF_STRING | (!(bool)csel ? MF_DISABLED | MF_GRAYED : 0), ID_INVERTSEL, L"&Invert selection");
					menu.AppendMenu(MF_SEPARATOR);

					menu.AppendMenu(MF_STRING | (is_cfg_Bookmarking() ? MF_UNCHECKED : MF_CHECKED), ID_PAUSE_BOOKMARKS, L"&Pause Bookmarking");
					menu.AppendMenu(MF_STRING, ID_PREF_PAGE, L"Con&figuration...");

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
					switch (cmd) {
					case ID_STORE:
						addBookmark();
						break;
					case ID_RESTORE:
						restoreFocusedBookmark();
						break;
					case  ID_RESET_TIME:
						g_masterList.at(isel).time = 0;
						m_guiList.ReloadItem(isel);
						m_guiList.UpdateItem(isel);
						break;
					case ID_DEL:
						m_guiList.RequestRemoveSelection();
						break;
					case ID_CLEAR:
						clearBookmarks();
						break;
					case ID_COPY:
						if (m_colActive[1]) {
							pfc::string8 coltext;
							for (auto i = 0; i < static_cast<int>(m_guiList.GetColumnCount()); i++) {
								m_guiList.GetColumnText(i, coltext);
								if (coltext.equals(COLUMNNAMES[1])) {
									pfc::string8 bookmark;
									m_guiList.GetSubItemText(m_guiList.GetSingleSel(), i, bookmark);

									ClipboardHelper::OpenScope scope;
									scope.Open(core_api::get_main_window(), true);
									ClipboardHelper::SetString(bookmark);
									scope.Close();

									break;
								}
							}
						}
						break;
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
					}
				}// contextmenu

			}
			catch (std::exception const& e) {
				FB2K_console_print_v("Context menu failure", e); //??
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

			configToUI();
		}
	private:

		// read dui config
		static void parseConfig(ui_element_config::ptr cfg, std::array<uint32_t, N_COLUMNS>& widths, std::array<bool, N_COLUMNS>& active) {

			FB2K_console_print_v("Parsing config");
			widths = defaultColWidths;
			active = defaultColActive;

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
				FB2K_console_print_v("Failed to parse configuration", e);
			}
			catch (exception_io_data e) {
				FB2K_console_print_v("Failed to parse configuration", e);
			}
		}

		static ui_element_config::ptr makeConfig(GUID ui_guid, std::array<uint32_t, N_COLUMNS> widths = defaultColWidths, const std::array<bool, N_COLUMNS> active = defaultColActive) {
			if (sizeof(widths) / sizeof(uint32_t) != N_COLUMNS)
				return makeConfig(ui_guid);

			FB2K_console_print_v("Making config from ",widths[0]," and ",widths[1]);

			ui_element_config_builder out;
			for (int i = 0; i < N_COLUMNS; i++)
				out << widths[i];
			for (int i = 0; i < N_COLUMNS; i++)
				out << active[i];
			return out.finish(ui_guid);
		}

		void configToUI() {

			FB2K_console_print_v("Applying config to UI: ", m_colWidths[0], " and ", m_colWidths[1]);
			auto DPI = m_guiList.GetDPI();

			if (m_guiList.GetHeaderCtrl() != NULL && m_guiList.GetHeaderCtrl().GetItemCount()) {
				m_guiList.DeleteColumns(pfc::bit_array_true(), false);
			}

			size_t ndx_tail = N_COLUMNS - 1;
			for (int i = 0; i < N_COLUMNS; i++) {
				FB2K_console_print_v("Config to UI: i is ", i, "; name: ", COLUMNNAMES[i], ", active: ", m_colActive[i], ", width: ", m_colWidths[i]);

				auto ndx_cont = !m_guiList.IsHeaderEnabled() ? 0 : m_guiList.GetColumnCount();
				if (m_colActive[i]) {
					int width = (m_colWidths[i] != 0 && m_colWidths[i] != ~0) ? m_colWidths[i] : defaultColWidths[i];	//use defaults instead of zero
					width = (std::min)(width, 1000);
					m_colContent[ndx_cont] = i;
					m_guiList.AddColumn(COLUMNNAMES[i], MulDiv(width, DPI.cx, 96));
				}
				else {
					//move to tail
					m_colContent[ndx_tail--] = i;
				}
			}
		}

	protected:

		const ui_element_instance_callback::ptr m_callback;
		const ui_element_config::ptr m_cfg;

		StyleManager* m_cust_stylemanager = nullptr;
		CListControlBookmark m_guiList;

	private:

		bool m_cui = false;

		fb2k::CDarkModeHooks m_dark;
		
		std::array<uint32_t, N_COLUMNS> m_colWidths;

		std::array<bool, N_COLUMNS> m_colActive;
		pfc::array_t<size_t> m_colContent;

		friend class CListControlBookmark;
	};
}
