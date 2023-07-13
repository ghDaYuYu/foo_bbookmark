#include "stdafx.h"

#include "resource.h"
#include "bookmark_core.h"
#include "bookmark_persistence.h"
#include "bookmark_worker.h"

#include <atltypes.h>
#include <libPPUI/CListControlSimple.h>
#include <libPPUI/win32_utility.h>
#include <libPPUI/win32_op.h> // WIN32_OP()
#include <helpers/atl-misc.h>
#include <helpers/BumpableElem.h>
#include <helpers/DarkMode.h>

#include <algorithm>
#include <array>
#include <vector>

#include <iomanip>
#include <fstream>

namespace {
	//~~~~~~~~~~~~~~~~~DEFINITIONS~~~~~~~~~~~~~~~~~~~
	static const GUID guid_bookmarkDialog = { 0x5c20d8ca, 0x91be, 0x4ef2, { 0xae, 0x81, 0x32, 0x1f, 0x1c, 0x2d, 0x27, 0x23 } };

	bookmark_persistence g_permStore;
	bookmark_worker g_bmWorker;

	//Config
	const enum colID {
		TIME_COL = 0,
		DESC_COL,
		PLAYLIST_COL,
		N_COLUMNS
	};
	static const char* COLUMNNAMES[] = { "Time", "Description", "Playlist" };
	static const std::array<uint32_t, N_COLUMNS> defaultColWidths = { 40, 150, 110 };
	static const std::array<bool, N_COLUMNS> defaultColActive = { true, true, false };

	class CListControlBookmarkDialog : public CDialogImpl<CListControlBookmarkDialog>, public ui_element_instance,
		private IListControlBookmarkSource {
	protected:
		const ui_element_instance_callback::ptr m_callback;
	public:
		//---Dialog Setup---
		CListControlBookmarkDialog(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb), m_guiList(this) {
			m_colActive = defaultColActive;
            m_colContent.resize(N_COLUMNS);
            parseConfig(cfg, m_colWidths, m_colActive);
		}

		enum { IDD = IDD_BOOKMARK_DIALOG };

		BEGIN_MSG_MAP_EX(CListControlBookmarkDialog)
			MSG_WM_INITDIALOG(OnInitDialog)
			MSG_WM_SIZE(OnSize)
			MSG_WM_CONTEXTMENU(OnContextMenu)
			END_MSG_MAP()

		void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
		HWND get_wnd() { return m_hWnd; }

		static GUID g_get_guid() {
			return guid_bookmarkDialog;
		}

		static GUID g_get_subclass() { return ui_element_subclass_utility; }
		static void g_get_name(pfc::string_base & out) { out = "Basic Bookmarks"; }
		static const char * g_get_description() { return "Provides basic bookmark functionality."; }


		// ---(Re)store Bookmarks---
		//Restores the bookmark currently focused by the first-born instance
		static void restoreFocusedBookmark() {
			if (g_guiLists.empty()) {	//fall back to 0 if there is no list, and hence no selected bookmark
				FB2K_console_print("Global Bookmark Restore: No bookmark UI found, falling back to first bookmark");
				restoreBookmark(0);
				return;
			}

			size_t focused;
			if (g_primaryGuiList != NULL) {
				FB2K_console_print("Global Bookmark Restore: No primary UI found, falling back to firstborn UI.");
				focused = g_primaryGuiList->GetFocusItem();
			} else {
				auto it = g_guiLists.begin();
				focused = (*it)->GetFocusItem();
			}
			restoreBookmark(focused);
		}

		//Restores the bookmark identified by index
		static void restoreBookmark(size_t index) { g_bmWorker.restore(g_masterList, index); }

		//Creates new bookmarks, updates all UIs, writes to file
		static void storeBookmark() {
			g_bmWorker.store(g_masterList);

			for (std::list<CListControlBookmark *>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				(*it)->OnItemsInserted(g_masterList.size(), 1, true);
			}

			if (cfg_bookmark_verbose) FB2K_console_print("Created Bookmark, saving to file now.");
			g_permStore.write(g_masterList);
		}

		//Deletes all bookmarks, updates all UIs, writes to file
		static void clearBookmarks() {
			size_t oldCount = g_masterList.size();
			g_masterList.clear();
			for (std::list<CListControlBookmark *>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
				(*it)->OnItemsRemoved(pfc::bit_array_true(), oldCount);
			}
			g_permStore.write(g_masterList);
		}

		~CListControlBookmarkDialog() {
			if (cfg_bookmark_verbose) FB2K_console_print("Destructor was called");
			if (g_primaryGuiList == &m_guiList) {
				g_primaryGuiList = NULL;
			}
			g_guiLists.remove(&m_guiList);
		}

	private:
		// ==================================members====================
		CListControlBookmark m_guiList;
		std::array<uint32_t, N_COLUMNS> m_colWidths;
		std::array<bool, N_COLUMNS> m_colActive;
		pfc::array_t<size_t> m_colContent;
		fb2k::CDarkModeHooks m_dark;

		//========================UI code===============================
		BOOL OnInitDialog(CWindow, LPARAM) {
			// Create replacing existing windows list control
			// automatically initialize position, font, etc
			m_guiList.CreateInDialog(*this, IDC_BOOKMARKLIST);
			
			// darkmode
            m_dark.AddDialogWithControls(*this);
            
			g_guiLists.emplace_back(&m_guiList);
			if (g_primaryGuiList == NULL) {
				g_primaryGuiList = &m_guiList;
			}

			HWND hwndBookmarkList = GetDlgItem(IDC_BOOKMARKLIST);
			CListViewCtrl wndList(hwndBookmarkList);
			wndList.SetParent(m_hWnd);

			configToUI();
			ShowWindow(SW_SHOW);

			return true; // system should set focus
		}

		void OnSize(UINT, CSize s) {
			m_guiList.ResizeClient(s.cx - 2, s.cy - 1, 1);
			//Making the guilist very slightly undersized makes it a bit easier to grab unto the column seperator when it's at the right edge

			//TODO: attempt to move m_guiList to the top left by one pixel to slim down its border
			//Cpoint newPos = Cpoint(0, 0);
			//m_guiList.MoveViewOrigin(newPos);
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
			g_permStore.write(g_masterList);
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

			g_permStore.write(g_masterList);	//Write to file
			return true;
		}
		void listItemAction(ctx_t, size_t item) override { restoreBookmark(item); }

		void listSubItemClicked(ctx_t, size_t item, size_t subItem) override { return; }
		void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char * val) override { return; }	//We don't want to allow edits		
		bool listIsColumnEditable(ctx_t, size_t subItem) override { return false; }

		void OnContextMenu(CWindow wnd, CPoint point) {
			try {
				if (m_callback->is_edit_mode_enabled()) {
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
					//Contextmenu for Headers: (Dis-)able columns
					const int stringlength = 25;
					for (uint32_t i = 0; i < N_COLUMNS; i++) {
						//Need to convert to UI friendly stringformat first:
						wchar_t wideString[stringlength];
						size_t outSize;
						mbstowcs_s(&outSize, wideString, stringlength, COLUMNNAMES[i], stringlength - 1);

						//The + 1 and - 1 below ensure that the 0-Index (which cmd is set to when clicking outside of the context menu) remains unused
						auto flags = MF_STRING;
						if (m_colActive[i])
							flags |= MF_CHECKED;
						else
							flags |= MF_UNCHECKED;

						menu.AppendMenu(flags, i + 1, wideString);
					}

					int cmd;
					{
						// Callback object to show menu command descriptions in the status bar.
						// it's actually a hidden window, needs a parent HWND, where we feed our control's HWND
						CMenuDescriptionMap descriptions(m_hWnd);

						// Set descriptions of all our items
						descriptions.Set(TIME_COL + 1, "Timestamp");
						descriptions.Set(DESC_COL + 1, "Custom Description");
						descriptions.Set(PLAYLIST_COL + 1, "Playlist");

						cmd = menu.TrackPopupMenuEx(TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD, point.x, point.y, descriptions, nullptr);
					}
					if (cmd) {
                        m_colActive[cmd - 1] = !m_colActive[cmd - 1]; //Toggle the state of the column whose menucommand was triggered
                        bool all_disabled = std::find(m_colActive.begin(), m_colActive.end(), true) == m_colActive.end();
                        if (all_disabled) {
                            //revert
                            m_colActive[cmd - 1] = !m_colActive[cmd - 1];
                        }
                        else {
                            configToUI();
                        }
                    }

				} else {
					//Contextmenu for listbody
					enum { ID_STORE = 1, ID_RESTORE, ID_DEL, ID_CLEAR, ID_SELECTALL, ID_SELECTNONE, ID_INVERTSEL, ID_MAKEPRIME };
					menu.AppendMenu(MF_STRING, ID_STORE, L"Store Bookmark");
					menu.AppendMenu(MF_STRING, ID_RESTORE, L"Restore Bookmark");
					menu.AppendMenu(MF_STRING, ID_DEL, L"Delete Selected Bookmarks");
					menu.AppendMenu(MF_SEPARATOR);
					menu.AppendMenu(MF_STRING, ID_CLEAR, L"Clear Bookmarks");
					menu.AppendMenu(MF_SEPARATOR);
					// Note: Ctrl+A handled automatically by CListControl, no need for us to catch it
					menu.AppendMenu(MF_STRING, ID_SELECTALL, L"Select all\tCtrl+A");
					menu.AppendMenu(MF_STRING, ID_SELECTNONE, L"Select none");
					menu.AppendMenu(MF_STRING, ID_INVERTSEL, L"Invert selection");
					menu.AppendMenu(MF_SEPARATOR);
					//Determine whether to set the checkmark:
					auto flags = MF_STRING;
					if (g_primaryGuiList == &m_guiList)
						flags |= MF_CHECKED;
					else
						flags |= MF_UNCHECKED;
					menu.AppendMenu(flags, ID_MAKEPRIME, L"Primary UI");

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
						storeBookmark();
						break;
					case ID_RESTORE:
						restoreFocusedBookmark();
						break;
					case ID_DEL:
						m_guiList.RequestRemoveSelection();
						break;
					case ID_CLEAR:
						clearBookmarks();
						break;
					case ID_SELECTALL:
						m_guiList.SelectAll();
						break;
					case ID_SELECTNONE:
						m_guiList.SelectNone();
						break;
					case ID_INVERTSEL:
					{
						auto mask = m_guiList.GetSelectionMask();
						m_guiList.SetSelection(
							// Items which we alter - all of them
							pfc::bit_array_true(),
							// Selection values - inverted original selection mask
							pfc::bit_array_not(mask)
						);
					}
					break;
					case ID_MAKEPRIME:
						g_primaryGuiList = &m_guiList;
						break;
					}
				}//Contextmenu for listbody

			} catch (std::exception const & e) {
				console::complain("Context menu failure", e); //rare
			}
		};


		// =================================config code=======================================
	public:
		static ui_element_config::ptr g_get_default_configuration() { return ui_element_config::g_create_empty(g_get_guid()); }
		//get: Derive config from state; called at shutdown
		ui_element_config::ptr get_configuration() {
			if (cfg_bookmark_verbose) FB2K_console_print("get_configuration called.");

			for (int i = 0; i < N_COLUMNS; i++) {
                auto col_ndx = m_colContent[i];
                if (i < m_guiList.GetColumnCount()) {
                    m_colWidths[col_ndx] = static_cast<int>(m_guiList.GetColumnWidthF(i));
                }
                else {
                    //default
                    m_colWidths[col_ndx] = defaultColWidths[col_ndx];
                }
            }

			return makeConfig(m_colWidths, m_colActive);
		}
		//set: Apply config to class
		void set_configuration(ui_element_config::ptr config) {
			if (cfg_bookmark_verbose) FB2K_console_print("set_configuration called.");
			parseConfig(config, m_colWidths, m_colActive);

			configToUI();
		}
	private:
		//Reads a config into the supplied variables; Falls back to defaults if necessary
		static void parseConfig(ui_element_config::ptr cfg, std::array<uint32_t, N_COLUMNS> &widths, std::array<bool, N_COLUMNS> &active) {
			if (cfg_bookmark_verbose) FB2K_console_print("Parsing config");
			for (int i = 0; i < N_COLUMNS; i++)
				widths[i] = defaultColWidths[i];
			for (int i = 0; i < N_COLUMNS; i++)
				active[i] = defaultColActive[i];

			try {
				::ui_element_config_parser configParser(cfg);
				//read from config:
				for (int i = 0; i < N_COLUMNS; i++)
					configParser >> widths[i];
				for (int i = 0; i < N_COLUMNS; i++)
					configParser >> active[i];
			} catch (exception_io_data_truncation e) {
				console::complain("Failed to parse configuration", e);
			} catch (exception_io_data e) {
				console::complain("Failed to parse configuration", e);
			}
		}

		static ui_element_config::ptr makeConfig(std::array<uint32_t, N_COLUMNS> widths = defaultColWidths, std::array<bool, N_COLUMNS> active = defaultColActive) {
			if (sizeof(widths) / sizeof(uint32_t) != N_COLUMNS)
				return makeConfig();

			if (cfg_bookmark_verbose) FB2K_console_print("Making config from ", widths[0], " and ", widths[1]);

			ui_element_config_builder out;
			for (int i = 0; i < N_COLUMNS; i++)
				out << widths[i];
			for (int i = 0; i < N_COLUMNS; i++)
				out << active[i];
			return out.finish(g_get_guid());
		}

		void configToUI() {
			if (cfg_bookmark_verbose) FB2K_console_print("Applying config to UI: ", m_colWidths[0], " and ", m_colWidths[1]);
			auto DPI = m_guiList.GetDPI();
			m_guiList.DeleteColumns(pfc::bit_array_true(), false);

			size_t ndx_tail = N_COLUMNS - 1;
			for (int i = 0; i < N_COLUMNS; i++) {
				if (cfg_bookmark_verbose) FB2K_console_print("configToUi: i is ", i, "; name: ", COLUMNNAMES[i], ", active: ", m_colActive[i], ", width: ", m_colWidths[i]);
				
				auto ndx_cont = !m_guiList.IsHeaderEnabled() ? 0 : m_guiList.GetColumnCount();
                if (m_colActive[i]) {
                    int width = (m_colWidths[i] != 0) ? m_colWidths[i] : defaultColWidths[i]; //use defaults instead of zero
                    m_colContent[ndx_cont] = i;
                    m_guiList.AddColumn(COLUMNNAMES[i], MulDiv(width, DPI.cx, 96));
                }
                else {
                    //move to tail
                    m_colContent[ndx_tail--] = i;
                }
			}
		}
	};

	//=========================service factory======================
	class clist_control_bookmark_impl : public ui_element_impl_withpopup<CListControlBookmarkDialog> {};
	static service_factory_t< clist_control_bookmark_impl > g_CListControlBookmarkDialog_factory;


	//=====================Callback Classes========================
	//---Play Callback---
	class bmCore_play_callback : public play_callback_static {
	public:
		/* play_callback methods go here */
		void on_playback_starting(play_control::t_track_command p_command, bool p_paused) override {}
		void on_playback_stop(play_control::t_stop_reason p_reason) override {}
		void on_playback_pause(bool p_state) override {}
		void on_playback_edited(metadb_handle_ptr p_track) override {}
		void on_playback_dynamic_info(const file_info & p_info) override {}
		void on_playback_dynamic_info_track(const file_info & p_info) override {}
		void on_volume_change(float p_new_val) override {}

		void on_playback_new_track(metadb_handle_ptr p_track) override {
			if (!cfg_bookmark_autosave_newTrack)
				return;

			if (g_bmAuto.upgradeDummy(g_masterList, g_guiLists))
				g_permStore.write(g_masterList);
			g_bmAuto.updateDummy();
		}
		void on_playback_seek(double p_time) override { g_bmAuto.updateDummyTime(); }
		void on_playback_time(double p_time) override { g_bmAuto.updateDummyTime(); }

		/* The play_callback_manager enumerates play_callback_static services and registers them automatically. We only have to provide the flags indicating which callbacks we want. */
		virtual unsigned get_flags() {
			return flag_on_playback_new_track | flag_on_playback_seek | flag_on_playback_time;
		}
	};

	static service_factory_single_t<bmCore_play_callback> g_play_callback_static_factory;

	//---Init/Quit Callback---
	class initquit_bbookmark : public initquit {
		virtual void on_init() {
			g_permStore.readDataFile(g_masterList);
		}

		virtual void on_quit() {
			if (cfg_bookmark_autosave_onQuit) {
				if (g_bmAuto.upgradeDummy(g_masterList, g_guiLists))
					g_permStore.write(g_masterList);
			}
		}
	};
	static initquit_factory_t< initquit_bbookmark > g_bookmark_initquit;

} //anonymous namespace

//==================Hooks for main menu=======================
void bbookmarkHook_store() { CListControlBookmarkDialog::storeBookmark(); }
void bbookmarkHook_restore() { CListControlBookmarkDialog::restoreFocusedBookmark(); }
void bbookmarkHook_clear() { CListControlBookmarkDialog::clearBookmarks(); }
