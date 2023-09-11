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

		CListControlBookmark* pdlg = (CListControlBookmark*)(ctx);

		auto subItemContent = pdlg->GetColContent(subItem);

		switch (subItemContent) {
		case TIME_COL:
		{
			std::ostringstream conv;
			int hours = (int)rec.get_time() / 3600;
			int minutes = (int)std::fmod(rec.get_time(), 3600) / 60;
			int seconds = (int)std::fmod(rec.get_time(), 60);
			if (hours != 0)
				conv << hours << ":";
			conv << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
			return conv.str().c_str();
		}
		case DESC_COL:
			return rec.desc.c_str();
		case PLAYLIST_COL:
			return rec.playlist.c_str();
		case ELU_COL:
			return rec.comment.c_str();
		case DATE_COL:
			return rec.date.c_str();
		default:
			return "";
		}
	}

	bool ILOD_BookmarkSource::listReorderItems(ctx_t, const size_t* order, size_t count) {

		PFC_ASSERT(count == g_masterList.size());
		pfc::reorder_t(g_masterList, order, count);
		CListCtrlMarkDialog::CancelUIListEdits();
		g_permStore.writeDataFile(glb::g_masterList);
		return true;
	}

	bool ILOD_BookmarkSource::listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) {
		size_t oldCount = g_masterList.size();

		pfc::remove_mask_t(g_masterList, mask); //remove from global list

		//Update all guiLists

		for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
			if ((*it) != ctx) {
				(*it)->OnItemsRemoved(mask, oldCount);
			}
		}
		CListCtrlMarkDialog::CancelUIListEdits();
		g_permStore.writeDataFile(glb::g_masterList);

		return true;
	}

	void ILOD_BookmarkSource::listItemAction(ctx_t ctx, size_t item) {

		CListCtrlMarkDialog::restoreBookmark(item);
	}

	uint32_t ILOD_BookmarkSource::listGetEditFlags(ctx_t ctx, size_t item, size_t subItem) {
		return listIsColumnEditable(ctx, subItem) ? 0 : InPlaceEdit::KFlagReadOnly;
	}

	bool ILOD_BookmarkSource::listIsColumnEditable(ctx_t ctx, size_t subItem) {
		if (!is_cfg_Bookmarking()) {
			CListControlBookmark* pdlg = (CListControlBookmark*)(ctx);
			auto subItemContent = pdlg->GetColContent(subItem);
			if (subItemContent == ELU_COL) {
				return true;
			}
		}
		return false;
	}

	void ILOD_BookmarkSource::listSubItemClicked(ctx_t ctx, size_t item, size_t subItem)
	{
		if (listIsColumnEditable(ctx, subItem)) {
			CListControlBookmark* pdlg = (CListControlBookmark*)(ctx);
			pdlg->TableEdit_Start(item, subItem);
		}
	}

	pfc::string8 ILOD_BookmarkSource::listGetEditField(ctx_t ctx, size_t item, size_t subItem, size_t& lineCount) {
		lineCount = 1; return listGetSubItemText(ctx, item, subItem);
	}

	void ILOD_BookmarkSource::listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) {
		auto& rec = g_masterList[item];
		rec.comment = pfc::string8(val);
		g_permStore.writeDataFile(glb::g_masterList);
	}
}