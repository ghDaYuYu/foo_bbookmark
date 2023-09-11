#include "stdafx.h"
#include <regex>
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

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);

		auto subItemContent = plc->GetColContent(subItem);

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

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);
		auto subItemContent = plc->GetColContent(subItem);

		if (subItemContent == ELU_COL || subItemContent == TIME_COL) {
			return true;
		}
		return false;
	}

	void ILOD_BookmarkSource::listSubItemClicked(ctx_t ctx, size_t item, size_t subItem)
	{
		if (listIsColumnEditable(ctx, subItem)) {
			CListControlBookmark* plc = (CListControlBookmark*)(ctx);
			plc->TableEdit_Start(item, subItem);
		}
	}

	pfc::string8 ILOD_BookmarkSource::listGetEditField(ctx_t ctx, size_t item, size_t subItem, size_t& lineCount) {

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);
		auto subItemContent = plc->GetColContent(subItem);

		if (subItemContent == ELU_COL) {
			lineCount = 2;
		}
		else if (subItemContent == TIME_COL) {
			lineCount = 1;
		}

		return listGetSubItemText(ctx, item, subItem);
	}

	size_t GetEditFieldSeconds(const char* val) {
	
		std::string field = val;

		std::smatch sm;
		std::regex regex_v("^(?:(?:([01]?\\d|2[0-3]):)?([0-5]?\\d):)?([0-5]?\\d)$");
		std::regex_match(field, sm, regex_v);

		std::vector<int> vhms;
		for (std::smatch::iterator it = sm.begin(); it != sm.end(); ++it) {

			const std::string tmpstr = it->str();
			bool is_num = pfc::string_is_numeric(tmpstr.c_str());

			if (is_num) {
				vhms.emplace_back(atoi(tmpstr.c_str()));
			}
		}

		size_t secs = SIZE_MAX;

		if (vhms.size() && vhms.size() <= 3) {

			secs = 0;
			secs += vhms[vhms.size() - 1];

			if (vhms.size() > 1) {
				secs += vhms[vhms.size() - 2] * 60;
			}
			if (vhms.size() > 2) {
				secs += vhms[vhms.size() - 3] * 3600;
			}
		}

		return secs;
	}

	void ILOD_BookmarkSource::listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) {

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);
		auto subItemContent = plc->GetColContent(subItem);

		if (subItemContent == TIME_COL) {

			size_t secs = GetEditFieldSeconds(val);

			if (secs == SIZE_MAX) {
				plc->TableEdit_Abort(false);
				return;
			}
			else {
				//replace time
				auto& rec = g_masterList[item];
				rec.set_time(static_cast<double>(secs));
				g_permStore.writeDataFile(g_masterList);
			}
		}
		else if (subItemContent == ELU_COL) {
			//replace comment
			auto& rec = g_masterList[item];
			rec.comment = pfc::string8(val);
			g_permStore.writeDataFile(glb::g_masterList);
		}
	}
}