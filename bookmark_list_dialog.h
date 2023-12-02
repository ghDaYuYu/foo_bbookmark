#pragma once

#include "resource.h"

#include <array>

#include <helpers/atl-misc.h>
#include <helpers/helpers.h>
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

#include "helpers/cfg_guidlist.h"

using namespace glb;

namespace dlg {

	static const char* COLUMNNAMES[] = { "#", "Time", "Bookmark", "Playlist", "Comment", "Date"};
	static const char BOOKMARK_COL = 2; //todo: config to ui & clipboard

	static const std::array<uint32_t, N_COLUMNS> default_cols_width = { 20, 40, 150, 110, 150, 150 };
	static const std::array<bool, N_COLUMNS> default_cols_active = { false, true, true, false, false, false };

	class CListCtrlMarkDialog : public CDialogImpl<CListCtrlMarkDialog>, private ILOD_BookmarkSource {

	public:

		//not overriding - serves cui_bmark
		HWND get_wnd() const { return m_hWnd; }

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

			m_cols_active = default_cols_active;
			m_cols_content.resize(N_COLUMNS);

			parseConfig(cfg, m_sorted_dir, m_cols_width, m_cols_active);

			m_cust_stylemanager->setChangeHandler([&] { this->on_style_change(); });
		}

		// CUI constructor

		CListCtrlMarkDialog(HWND parent, bool sorted_dir, std::array<uint32_t, N_COLUMNS> colWidths, std::array<bool, N_COLUMNS> colActive)
			: m_sorted_dir(sorted_dir), m_cols_width(colWidths), m_cols_active(colActive), m_guiList(this, true), m_cust_stylemanager(new CuiStyleManager())
		{

			m_cui = true;

			if (!colWidths.size()) {
				m_cols_active = default_cols_active;
			}

			m_cols_content.resize(N_COLUMNS);
			
			parseConfig(nullptr, m_sorted_dir, m_cols_width, m_cols_active);
			m_cust_stylemanager->setChangeHandler([&] { this->on_style_change(); });

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
			return makeConfig(guid_dui_bmark, false);
		}

		// Restore Bookmarks

		static void restoreFocusedBookmark() {

			if (g_guiLists.empty() || (g_primaryGuiList && g_primaryGuiList->GetFocusItem() == pfc_infinite)) {
				FB2K_console_print_v("Global Bookmark Restore: No bookmark UI found, falling back to last bookmark");

				if (g_store.Size()) {
					restoreBookmark(g_store.Size() -1);
				}
				return;
			}

			CListControlBookmark* p_list = nullptr;
			if (!g_primaryGuiList) {
				p_list = g_primaryGuiList;
			}
			else {
				FB2K_console_print_v("Global Bookmark Restore: No primary UI found, falling back to firstborn UI.");
				p_list = *g_guiLists.begin();
			}

			if (p_list) {

				size_t focused = p_list->GetFocusItem();

				if (focused != SIZE_MAX) {
					bool sorted = p_list->GetSortOrder();
					if (p_list->GetItemCount() && sorted) {
						focused = p_list->GetItemCount() - 1 - focused;
					}
					restoreBookmark(focused);
				}
			}
		}

		// Restores bookmark identified by index

		static void restoreBookmark(size_t index) {
			bookmark_worker bmWorker;
			bmWorker.restore(index);
		}

		static void addBookmark() {

			CancelUIListEdits();

			bookmark_worker bmWorker;
			bmWorker.store(g_bmAuto.getDummy());

			for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				size_t item = (std::max)(0, (int)g_store.Size() - 1);
				if ((*it)->GetSortOrder()) {
					item = 0;
				}
				(*it)->SelectNone();
				(*it)->OnItemsInserted(item, 1, true);
				(*it)->EnsureItemVisible(item, false);
				(*it)->SetFocusItem(item);
			}

			FB2K_console_print_v("Created Bookmark, saving to file...");

			g_store.Write();
		}

		static void clearBookmarks() {

			CListCtrlMarkDialog::CancelUIListEdits();
			g_store.Clear();

			for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				(*it)->OnItemsRemoved(pfc::bit_array_true(),g_store.Size());
			}
			
			g_store.Write();
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
				|| g_store.Size();
		}

		static bool canClear() {
			return (g_primaryGuiList && (bool)g_primaryGuiList->GetItemCount())
				|| g_store.Size();
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
			m_guiList.Initialize(&m_cols_content);
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

				CPoint ptInvalid(-1, -1);
				if (CPoint(point) == ptInvalid) {
					//no items in list
					::GetCursorPos(&point);
				}

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
						if (m_cols_active[i])
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
						descriptions.Set(ITEM_NUMBER + 1, "Item order");
						descriptions.Set(TIME_COL + 1, "Playback timestamp");
						descriptions.Set(DESC_COL + 1, "Custom bookmark description");
						descriptions.Set(PLAYLIST_COL + 1, "Playlist");
						descriptions.Set(ELU_COL + 1, "Comment");
						descriptions.Set(DATE_COL + 1, "Bookmark date");

						cmd = menu.TrackPopupMenuEx(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, descriptions, nullptr);
					}

					if (cmd) {

						GetRuntimeColumnWidths(m_cols_width);

						size_t colndx = static_cast<size_t>(cmd) - 1;
						m_cols_active[colndx] = !m_cols_active[colndx]; // toggle column state whose menucommand was triggered
						bool all_disabled = std::find(m_cols_active.begin(), m_cols_active.end(), true) == m_cols_active.end();
						if (all_disabled) {
							//revert
							m_cols_active[colndx] = !m_cols_active[colndx];
						}
						else {
							if (!m_cols_active[colndx]) {
								//keep width before disabling
								auto col_ndx = m_cols_content[colndx];
								m_cols_width[col_ndx] = static_cast<int>(m_guiList.GetColumnWidthF(colndx));
							}
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

					if (csel && m_guiList.GetSortOrder()) {
						bit_array_bittable tmp_mask(selmask);
						for (size_t i = 0; i < selmask.size(); i++) {
							selmask.set(selmask.size() - 1 - i, tmp_mask.get(i));
						}
						if (isel != SIZE_MAX) {
							isel = selmask.size() - 1 - isel;
						}
					}

					bool bsinglesel = m_guiList.GetSingleSel() != SIZE_MAX;
					auto playlist_api = playlist_manager_v5::get();

					bool bactive_playlist = playlist_api->get_active_playlist() != SIZE_MAX;

					bool bresetable_time = false, bresetable_playlist = false, bresetable_comment = false, bassignable = false;

					if (bsinglesel) {
						const bookmark_t rec = g_store.GetItem(isel);
						bresetable_time =  rec.get_time();
						bresetable_playlist = rec.playlist.get_length();
						bresetable_comment = rec.comment.get_length();
					}
					
					bassignable = (bool)icount && (bsinglesel ||csel > 1) && bactive_playlist;

					bresetable_time |= csel > 1;
					bresetable_playlist |= csel > 1;
					bresetable_comment |= csel > 1;

					//Contextmenu for listbody
					enum { ID_STORE = 1, ID_RESTORE, ID_RESET_TIME, ID_ASSIGN_PLAYLIST, ID_RESET_PLAYLIST, ID_RESET_COMMENT, ID_DEL, ID_CLEAR,
						ID_COPY_BOOKMARK, ID_CMD_COPY, ID_COPY_PATH, ID_CMD_OPEN_FOLDER, ID_SELECTALL, ID_SELECTNONE, ID_INVERTSEL, ID_MAKEPRIME,
						ID_PAUSE_BOOKMARKS, ID_PREF_PAGE, ID_CMD_SEL_PROPERTIES
					};
					menu.AppendMenu(MF_STRING | (!CListCtrlMarkDialog::canStore() ? MF_DISABLED | MF_GRAYED : 0), ID_STORE, L"&Add bookmark");
					menu.AppendMenu(MF_STRING | (!bsinglesel ? MF_DISABLED | MF_GRAYED : 0), ID_RESTORE, L"R&estore\tENTER");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (!bupdatable || !bresetable_time ? MF_DISABLED | MF_GRAYED : 0), ID_RESET_TIME, L"Reset &time");
					menu.AppendMenu(MF_STRING | (!bupdatable || !bresetable_playlist ? MF_DISABLED | MF_GRAYED : 0), ID_RESET_PLAYLIST, L"Reset pla&ylist");
					menu.AppendMenu(MF_STRING | (!bupdatable || !bresetable_comment ? MF_DISABLED | MF_GRAYED : 0), ID_RESET_COMMENT, L"Reset co&mment");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (!bupdatable || !bassignable ? MF_DISABLED | MF_GRAYED : 0), ID_ASSIGN_PLAYLIST, L"A&ssign active playlist");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (!bupdatable || !(bool)csel ? MF_DISABLED | MF_GRAYED : 0), ID_DEL, L"&Remove\tDel");
					menu.AppendMenu(MF_SEPARATOR);
					//menu.AppendMenu(MF_STRING | (!bupdatable || !(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_CLEAR, L"C&lear all");
					//menu.AppendMenu(MF_SEPARATOR);
					if (bsinglesel) {
						menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_CMD_COPY, L"&Copy");
						menu.AppendMenu(MF_STRING | (!(bool)icount || !m_cols_active[1] ? MF_DISABLED | MF_GRAYED : 0), ID_COPY_BOOKMARK, L"Copy &bookmark");
						menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_COPY_PATH, L"Copy pat&h");
						menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_CMD_OPEN_FOLDER, L"&Open containing folder");
						menu.AppendMenu(MF_SEPARATOR);
					}
					// Note: Ctrl+A handled automatically by CListControl, no need for us to catch it
					menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_SELECTALL, L"Select &all\tCtrl+A");
					menu.AppendMenu(MF_STRING | (!(bool)icount ? MF_DISABLED | MF_GRAYED : 0), ID_SELECTNONE, L"C&lear selection");
					menu.AppendMenu(MF_STRING | (!(bool)csel ? MF_DISABLED | MF_GRAYED : 0), ID_INVERTSEL, L"&Invert selection");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (is_cfg_Bookmarking() ? MF_UNCHECKED : MF_CHECKED), ID_PAUSE_BOOKMARKS, L"&Pause bookmarking");
					menu.AppendMenu(MF_STRING, ID_PREF_PAGE, L"Vital Bookmarks pre&ferences...");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING | (!bsinglesel ? MF_DISABLED | MF_GRAYED : 0), ID_CMD_SEL_PROPERTIES, L"Properties\tAlt+ENTER");

					int cmd;
					{
						CMenuDescriptionMap descriptions(m_hWnd);

						descriptions.Set(ID_STORE, "This stores the playback position to a bookmark");
						descriptions.Set(ID_RESTORE, "This restores the playback position from a bookmark");
						descriptions.Set(ID_DEL, "This deletes all selected bookmarks");
						descriptions.Set(ID_SELECTALL, "Selects all items");
						descriptions.Set(ID_SELECTNONE, "Deselects all items");
						descriptions.Set(ID_INVERTSEL, "Invert selection");
						descriptions.Set(ID_ASSIGN_PLAYLIST, "Drop selected bookmark the active playlist then reassign playlist ");
						//descriptions.Set(ID_INVERTSEL, "The primary list's selection determines the bookmark restored by the global restore command.");

						cmd = menu.TrackPopupMenuEx(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, descriptions, nullptr);
					}

					switch (cmd) {

					case ID_STORE:
						addBookmark();
						break;
					case ID_RESTORE:
						restoreBookmark(isel);
						break;
					case ID_ASSIGN_PLAYLIST:
					[[fallthrough]];
					case ID_RESET_TIME:
					[[fallthrough]];
					case ID_RESET_PLAYLIST:
					[[fallthrough]];
					case ID_RESET_COMMENT: {
						size_t c = m_guiList.GetItemCount();
						size_t f = selmask.find(true, 0, c);
						for (size_t w = f; w < c; w = selmask.find(true, w + 1, c)) {
							bookmark_t rec = g_store.GetItem(w);
							if (cmd == ID_RESET_TIME) {
								
								rec.set_time(0.0);
								g_store.SetItem(w, rec);
							}
							else if (cmd == ID_RESET_PLAYLIST) {
								rec.playlist = "";
								rec.guid_playlist = pfc::guid_null;
								g_store.SetItem(w, rec);
							}
							else if (cmd == ID_ASSIGN_PLAYLIST) {

								GUID guid;
								pfc::string8 buffer;
								size_t act_ndx = playlist_api->get_active_playlist();
								playlist_api->playlist_get_name(act_ndx, buffer);
								guid = playlist_api->playlist_get_guid(act_ndx);

								rec.playlist = buffer;
								rec.guid_playlist = guid;
								g_store.SetItem(w, rec);
							}
							else if (cmd == ID_RESET_COMMENT) {
								rec.comment = "";
								g_store.SetItem(w, rec);
							}
						}

						//ReloadItems and UpdateItems require unsorted selection mask
						auto directmask = m_guiList.GetSelectionMask();
						m_guiList.ReloadItems(directmask);
						m_guiList.UpdateItems(directmask);

						break;
					}
					case ID_DEL:
						m_guiList.RequestRemoveSelection();
						break;

					case ID_COPY_BOOKMARK:
					[[fallthrough]];
					case ID_COPY_PATH: {

						if (!clip_bookmark.get_length()) {
							clip_bookmark = g_masterList.at(isel).path;
							foobar2000_io::extract_native_path(clip_bookmark, clip_bookmark);
						}

						ClipboardHelper::OpenScope scope;
						scope.Open(core_api::get_main_window(), true);
						ClipboardHelper::SetString(fall_clip_text);
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
						auto tmpMask = m_guiList.GetSelectionMask();
						m_guiList.SetSelection(
							// Items which we alter - all of them
							pfc::bit_array_true(),
							// Selection values - inverted original selection mask
							pfc::bit_array_not(tmpMask)
						);
						break;
					}
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
					case ID_CMD_OPEN_FOLDER:
					[[fallthrough]];
					case ID_CMD_SEL_PROPERTIES:
					[[fallthrough]];
					case ID_CMD_COPY: {
						GUID fall_guid_ctx = pfc::guid_null;
						menu_helpers::name_to_guid_table menu_table;
						if (cmd == ID_CMD_OPEN_FOLDER) {
							bool bf = menu_table.search("Open containing folder", 22, fall_guid_ctx);
						}
						else if (cmd == ID_CMD_SEL_PROPERTIES) {
							bool bf = menu_table.search("Properties", 10, fall_guid_ctx);
						}
						else if (cmd == ID_CMD_COPY) {
							bool bf = menu_table.search("Copy", 4, fall_guid_ctx);
						}

						if (pfc::guid_equal(fall_guid_ctx, pfc::guid_null)) {
							break; //EXIT
						}

						auto rec = g_store.GetItem(isel);
						const char* path = rec.path;
						const t_uint32 subsong = rec.subsong;
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

						menu_helpers::run_command_context(fall_guid_ctx, pfc::guid_null, valid_handles);
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
				auto col_ndx = m_cols_content[i];
				if (i < static_cast<int>(m_guiList.GetColumnCount())) {
					m_cols_width[col_ndx] = static_cast<int>(m_guiList.GetColumnWidthF(i));
				}
				else {
					//default
					m_cols_width[col_ndx] = default_cols_width[col_ndx];
				}
			}

			return makeConfig(ui_guid, m_guiList.GetSortOrder(), m_cols_width, m_cols_active);
		}

		// host to dlg

		void set_configuration(ui_element_config::ptr config) {
			FB2K_console_print_v("set_configuration called.");
			parseConfig(config, m_sorted_dir, m_cols_width, m_cols_active);

			configToUI(false);
		}

		//todo: merge makeConfig

		void GetRuntimeColumnWidths(std::array<uint32_t, N_COLUMNS> & tmp_cols_width) const {
			for (int i = 0; i < N_COLUMNS; i++) {
				auto col_ndx = m_cols_content[i];
				if (i < static_cast<int>(m_guiList.GetColumnCount())) {
					tmp_cols_width[col_ndx] = static_cast<int>(m_guiList.GetColumnWidthF(i));
				}
				else {
					//default
					tmp_cols_width[col_ndx] = default_cols_width[col_ndx];
				}
			}
		}

		void get_uicfg(stream_writer_formatter<>* out, abort_callback& p_abort/*, const std::array<uint32_t, N_COLUMNS>& cols_width, const std::array<bool, N_COLUMNS>& cols_active*/) const {
			std::array<uint32_t, N_COLUMNS> tmp_cols_width/* = cols_width*/;

			GetRuntimeColumnWidths(tmp_cols_width);

			pfc::string8 strVer; strVer << kUI_CONF_VER;
			*out << (pfc::string_base&)strVer;

			for (int i = 0; i < N_COLUMNS; i++) {
				*out << tmp_cols_width[i];
			}
			for (int i = 0; i < N_COLUMNS; i++) {
				*out << m_cols_active[i];
			}

			*out << m_guiList.GetSortOrder();
		}

		// todo: merge parseConfig
		static void set_cui_config(stream_reader* p_reader, t_size p_size, abort_callback& p_abort, bool &bsort, std::array<uint32_t, N_COLUMNS>& cols_width, std::array<bool, N_COLUMNS>& cols_active) {
	
			size_t cfg_ver = 0;

			stream_reader_buffered srb(p_reader, p_size);
			pfc::mem_block mb; mb.resize(1024);

			pfc::array_t<t_uint8> buffer;
			buffer.set_size(p_size);

			p_reader->read(buffer.get_ptr(), p_size, p_abort);

			stream_reader_memblock_ref srmr(buffer);

			stream_reader_formatter<false> reader(srmr, p_abort);
			try {
				pfc::string8 ver;
				reader >> ver;
				cfg_ver = atoi(ver);
			}
			catch (exception_io_data_truncation e) {
				//v0 (VB 1.1)
			}

			//rewind
			srmr.reset();

			if (cfg_ver == 0) {
				for (int i = 1; i < dlg::N_COLUMNS; i++) {
					reader >> (t_uint32)cols_width[i];
				}
				for (int i = 1; i < dlg::N_COLUMNS; i++) {
					reader >> (bool)cols_active[i];
				}
			}
			else {
				pfc::string8 strVer;
				reader >> strVer;
				for (int i = 0; i < dlg::N_COLUMNS; i++) {
					reader >> (t_uint32)cols_width[i];
				}
				for (int i = 0; i < dlg::N_COLUMNS; i++) {
					reader >> (bool)cols_active[i];
				}

				reader >> bsort;

			}
		}

	private:

		//todo: merge set_cui_config
		static void parseConfig(ui_element_config::ptr cfg, bool & bsort, std::array<uint32_t, N_COLUMNS>& widths, std::array<bool, N_COLUMNS>& active) {

			FB2K_console_print_v("Parsing config");

			if (!widths.size()) {
				bsort = false;
				widths = default_cols_width;
				active = default_cols_active;
			}
			else {
				//..
			}

			if (!cfg.get_ptr()) {
				//cui
				return;
			}

			size_t cfg_ver = 0;
			size_t cfg_ver_col_diff = 1;

			try {

				::ui_element_config_parser test_configParser(cfg);
				try {
					pfc::string8 strVer;
					test_configParser >> strVer;
					cfg_ver = atoi(strVer);
				}
				catch (exception_io_data_truncation e) {
					//v0 (VB 1.1)
				}

				::ui_element_config_parser configParser(cfg);
				if (cfg_ver == 0) {
					for (int i = 1; i < N_COLUMNS; i++)
						configParser >> widths[i];
					for (int i = 1; i < N_COLUMNS; i++) {
						configParser >> active[i];
					}
				}
				else {
					if (cfg_ver != kUI_CONF_VER) { 
						exception_io_data_truncation e;
						throw e;
					}
					pfc::string8 ver;
					configParser >> ver; //todo: skip instead
					for (int i = 0; i < N_COLUMNS; i++)
						configParser >> widths[i];
					for (int i = 0; i < N_COLUMNS; i++) {
						configParser >> active[i];
					}

					configParser >> bsort;
				}
			}
			catch (exception_io_data_truncation e) {
				FB2K_console_print_e("Failed to parse configuration", e);
			}
			catch (exception_io_data e) {
				FB2K_console_print_e("Failed to parse configuration", e);
			}
		}

		//todo: merge get_uicfg 
		static ui_element_config::ptr makeConfig(GUID ui_guid, bool bsort, std::array<uint32_t, N_COLUMNS> widths = default_cols_width, const std::array<bool, N_COLUMNS> active = default_cols_active) {
	
			if (sizeof(widths) / sizeof(uint32_t) != N_COLUMNS) {
				return makeConfig(ui_guid, false);
			}

			FB2K_console_print_v("Making config from ", widths[0], " and ", widths[1]);

			ui_element_config_builder out;

			pfc::string8 strVer; strVer << kUI_CONF_VER;
			out << (pfc::string_base&)strVer;

			for (int i = 0; i < N_COLUMNS; i++)
				out << widths[i];
			for (int i = 0; i < N_COLUMNS; i++)
				out << active[i];

			out << bsort;

			return out.finish(ui_guid);
		}

		void configToUI(bool breload) {

			FB2K_console_print_v("Applying config to UI");


			auto DPI = m_guiList.GetDPI();

			if (m_guiList.GetHeaderCtrl() != NULL && m_guiList.GetHeaderCtrl().GetItemCount()) {
				m_guiList.ResetColumns(false);
			}

			auto fit = std::find(m_cols_active.begin(), m_cols_active.end(), true);
			if (fit == m_cols_active.end()) {
				m_cols_active[BOOKMARK_COL-1] = true;
				m_cols_active[BOOKMARK_COL] = true;
			}

			m_guiList.SetSortOrder(m_sorted_dir);

			size_t ndx_tail = N_COLUMNS - 1;
			for (int i = 0; i < N_COLUMNS; i++) {
				auto ndx_cont = !m_guiList.IsHeaderEnabled() ? 0 : m_guiList.GetColumnCount();
				if (m_cols_active[i]) {

					size_t width = (m_cols_width[i] != 0 && m_cols_width[i] != pfc_infinite) ? m_cols_width[i] : default_cols_width[i];	//use defaults instead of zero
					width = pfc::min_t<size_t>(width, 1000);
					m_cols_content[ndx_cont] = i;
					m_guiList.AddColumn(COLUMNNAMES[i], MulDiv(static_cast<int>(width), DPI.cx, 96), LVCFMT_LEFT, false);
				}
				else {
					//move to tail
					m_cols_content[ndx_tail--] = i;
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

		bool m_sorted_dir = false;
		std::array<uint32_t, N_COLUMNS> m_cols_width = {0};
		std::array<bool, N_COLUMNS> m_cols_active;
		pfc::array_t<size_t> m_cols_content;

		friend class CListControlBookmark;
	};
}
