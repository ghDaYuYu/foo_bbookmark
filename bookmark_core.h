#pragma once
#include <mutex>

#include "bookmark_types.h"
#include "bookmark_list_control.h"
#include "bookmark_automatic.h"
#include "bookmark_persistence.h"
#include "bookmark_preferences.h"

inline bookmark_persistence g_permStore;

//The masterList, containing all bookmarks during runtime
inline std::vector<bookmark_t> g_masterList;

inline std::list<CListControlBookmark*> g_guiLists;
inline CListControlBookmark* g_primaryGuiList = NULL;

inline CListControlBookmark* GetPrimaryGuiList() { return g_primaryGuiList; }

inline bookmark_automatic g_bmAuto;

inline HWND g_wnd_bookmark_pref = NULL;

inline std::mutex g_mtx_restoring;
inline bool g_restoring = false;

inline constexpr UINT UMSG_NEW_TRACK = WM_USER + 1001;
inline constexpr UINT UMSG_PAUSED = WM_USER + 1002;

