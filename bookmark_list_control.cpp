#include "stdafx.h"

#include <iomanip>

#include "bookmark_list_control.h"
#include "bookmark_list_dialog.h"

using namespace glb;

namespace dlg {

	size_t ILOD_BookmarkSource::listGetItemCount(ctx_t) {

		return g_masterList.size();
	}

	pfc::string8 ILOD_BookmarkSource::listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) {

		auto& rec = g_masterList[item];

		CListControlBookmark* pdlg = static_cast<CListControlBookmark*>(ctx);

		auto subItemContent = pdlg->GetColContent(subItem);

		switch (subItemContent) {
		case TIME_COL:
		{
			std::ostringstream conv;
			int hours = (int)rec.time / 3600;
			int minutes = (int)std::fmod(rec.time, 3600) / 60;
			int seconds = (int)std::fmod(rec.time, 60);
			if (hours != 0)
				conv << hours << ":";
			conv << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
			return conv.str().c_str();
		}
		case DESC_COL:
			return rec.desc.c_str();
		case PLAYLIST_COL:
			return rec.playlist.c_str();
		default:
			return "";
		}
	}

	bool ILOD_BookmarkSource::listReorderItems(ctx_t, const size_t* order, size_t count) {

		PFC_ASSERT(count == g_masterList.size());
		pfc::reorder_t(g_masterList, order, count);
		g_permStore.writeDataFileJSON(g_masterList);
		return true;
	}

	bool ILOD_BookmarkSource::listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) {
		size_t oldCount = g_masterList.size();

		pfc::remove_mask_t(g_masterList, mask); //remove from global list

		for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
			if ((*it) != ctx) {
				(*it)->OnItemsRemoved(mask, oldCount);
			}
		}

		g_permStore.writeDataFileJSON(g_masterList);
		return true;
	}

	void ILOD_BookmarkSource::listItemAction(ctx_t ctx, size_t item) {

		CListCtrlMarkDialog::restoreBookmark(item);
	}

}