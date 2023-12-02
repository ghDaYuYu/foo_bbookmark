#pragma once

#include <mutex>

#include "bookmark_types.h"
#include "bookmark_automatic.h"
#include "bookmark_store.h"
#include "bookmark_persistence.h"
#include "bookmark_preferences.h"

namespace dlg {

	class CListControlBookmark;

}

namespace glb {

	inline std::vector<bookmark_t> g_masterList;

	inline std::list<dlg::CListControlBookmark*> g_guiLists;

	inline bookmark_automatic g_bmAuto;

	inline HWND g_wnd_bookmark_pref = NULL;

	inline bookmark_store g_store; //masterList
	inline bookmark_persistence g_file; //JSON file

	inline dlg::CListControlBookmark* g_primaryGuiList = NULL;

	inline dlg::CListControlBookmark* GetPrimaryGuiList() {
		return g_primaryGuiList;
	}

	inline constexpr UINT UMSG_NEW_TRACK = WM_USER + 1001;
	inline constexpr UINT UMSG_PAUSED = WM_USER + 1002;
}
