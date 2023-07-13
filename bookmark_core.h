#pragma once
namespace {
	#include "bookmark_types.h"
	#include "bookmark_list_control.h"
	#include "bookmark_automatic.h"
	#include "bookmark_preferences.h"
	
	//The masterList, containing all bookmarks during runtime
	inline std::vector<bookmark_t> g_masterList = std::vector<bookmark_t>();
	inline std::list<CListControlBookmark*> g_guiLists;
	inline CListControlBookmark* g_primaryGuiList = NULL;
	
	inline bookmark_automatic g_bmAuto;
}
