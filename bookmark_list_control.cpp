#include "stdafx.h"
#include <regex>
#include <iomanip>

#include "bookmark_core.h"
#include "bookmark_store.h"

#include "bookmark_list_control.h"
#include "bookmark_list_dialog.h"

namespace dlg {

	using namespace glb;

	// ILOD overrides

	size_t ILOD_BookmarkSource::listGetItemCount(ctx_t) {
		return g_store.Size();
	}

	pfc::string8 ILOD_BookmarkSource::listGetSubItemText(ctx_t ctx, size_t item, size_t subItem) {

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);

		if (plc->GetSortOrder()) {
			item = listGetItemCount(ctx) - 1 - item;
		}

		const bookmark_t rec = g_store.GetMasterList()[item];

		auto subItemContent = plc->GetColContent(subItem);

		colID subItemcol = static_cast<colID>(subItemContent);

		switch (subItemcol) {
		case colID::ITEM_NUMBER:
			return std::to_string(item + 1).c_str();
		case colID::TIME_COL:
		{
			std::ostringstream conv;
			int hours = (int)(rec.get_time() / 3600);
			int minutes = (int)(std::fmod(rec.get_time(), 3600) / 60);
			int seconds = (int)(std::fmod(rec.get_time(), 60));
			if (hours != 0)
				conv << hours << ":";
			conv << std::setfill('0') << std::setw(2) << minutes << ":" << std::setfill('0') << std::setw(2) << seconds;
			return conv.str().c_str();
		}
		case colID::DESC_COL:
			return rec.get_name(true); //rec.desc.c_str();
		case colID::PLAYLIST_COL:
			return rec.playlist.c_str();
		case colID::ELU_COL:
			return rec.comment.c_str();
		case colID::DATE_COL:
			return rec.runtime_date.c_str();
		default:
			return "";
		}
	}

	bool ILOD_BookmarkSource::listReorderItems(ctx_t ctx, const size_t* order, size_t count) {

		auto masterList = g_store.GetMasterList();

		PFC_ASSERT(count == g_store.Size());

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);

		if (plc->GetSortOrder()) {
			size_t ci = listGetItemCount(ctx);
			pfc::array_t<t_size> order_data;
			order_data.resize(ci);
			for (size_t i = 0; i < ci; i++) {
				order_data[ci - 1 - i] = ci - 1 - *(order + i);
			}
			g_store.Reorder(order_data, ci);
		}
		else {
			pfc::array_t<t_size> order_data;
			order_data.append_fromptr(order, count);
			g_store.Reorder(order_data, count);
		}

		CListCtrlMarkDialog::CancelUIListEdits();

		g_store.Write();
		return true;
	}

	bool ILOD_BookmarkSource::listRemoveItems(ctx_t ctx, pfc::bit_array const& mask) {
		
		size_t oldCount = g_store.Size();

		bit_array_bittable sorted_mask((const bit_array_bittable&)mask);
		pfc::bit_array_bittable new_mask = sorted_mask;

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);

		//remove from global list

		if (plc->GetSortOrder()) {
			plc->GetSortOrderedMask(sorted_mask);
			new_mask = sorted_mask;
		}

		g_bmAuto.checkDeletedRestoredDummy(new_mask, oldCount);

		g_store.Remove(new_mask);

		//Update all guiLists

		for (std::list<CListControlBookmark*>::iterator it = g_guiLists.begin(); it != g_guiLists.end(); ++it) {
			if ((*it) != ctx) {
				(*it)->OnItemsRemoved(new_mask, oldCount);
			}
		}

		CListCtrlMarkDialog::CancelUIListEdits();

		g_store.Write();

		return true;
	}

	void ILOD_BookmarkSource::listItemAction(ctx_t ctx, size_t item) {

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);
		if (plc->GetSortOrder()) {
			item = listGetItemCount(ctx) - 1 - item;
		}

		CListCtrlMarkDialog::restoreBookmark(item);
	}

	uint32_t ILOD_BookmarkSource::listGetEditFlags(ctx_t ctx, size_t item, size_t subItem) {
		return listIsColumnEditable(ctx, subItem) ? 0 : InPlaceEdit::KFlagReadOnly;
	}

	bool ILOD_BookmarkSource::listIsColumnEditable(ctx_t ctx, size_t subItem) {

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);
		auto subItemContent = plc->GetColContent(subItem);

		if (subItemContent == colcast(colID::DESC_COL) ||
			subItemContent == colcast(colID::ELU_COL) ||
			subItemContent == colcast(colID::TIME_COL)) {

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

		if (subItemContent == colcast(colID::DESC_COL)) {
			//..
		}
		else if (subItemContent == colcast(colID::ELU_COL)) {
			lineCount = 2;
		}
		else if (subItemContent == colcast(colID::TIME_COL)) {
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

	bool gen_desc(bookmark_t & rec) {

		bool bres = false;

		pfc::string_formatter songDesc;
		titleformat_object::ptr desc_format;
		static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(desc_format, cfg_desc_format.get_value().c_str());

		if (rec.path.get_length() && !rec.path.startsWith("https://")) {
			abort_callback_impl p_abort;
			try {
				if (!filesystem_v3::g_exists(rec.path.c_str(), p_abort)) {
					FB2K_console_print_e("Create description failed...object not found.");
					return false;
				}
			}
			catch (exception_aborted) {
				return false;
			}

			auto metadb_ptr = metadb::get();
			metadb_handle_ptr track_bm = metadb_ptr->handle_create(rec.path.c_str(), rec.subsong);

			if (!track_bm->format_title(NULL, songDesc, desc_format, NULL)) {
				FB2K_console_print_e("Description format failed.");
				return false;
			}

			rec.desc = songDesc;
			bres = true;
		} // end check path

		return bres;
	}

	void ILOD_BookmarkSource::listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) {

		CListControlBookmark* plc = (CListControlBookmark*)(ctx);
		if (plc->GetSortOrder()) {
			item = listGetItemCount(ctx) - 1 - item;
		}

		auto subItemContent = plc->GetColContent(subItem);

		if (subItemContent == colcast(colID::DESC_COL)) {

			pfc::string8 buffer(val);
			bookmark_t rec = g_store.GetItem(item);
			if (!stricmp_utf8(buffer, rec.desc)) {
				rec.set_name("");
			}
			else if (!buffer.get_length()) {
				rec.set_name("");
				//regen desc
				gen_desc(rec);
			}
			else {
				rec.set_name(buffer);
			}

			g_store.SetItem(item, rec);
			g_store.Write();
		}
		else if (subItemContent == colcast(colID::TIME_COL)) {

			size_t secs = GetEditFieldSeconds(val);

			if (secs == SIZE_MAX) {
				plc->TableEdit_Abort(false);
				return;
			}
			else {
				//replace time
				bookmark_t rec = g_store.GetItem(item);

				rec.set_time(static_cast<double>(secs));
				g_store.SetItem(item, rec);

				g_store.Write();
			}
		}
		else if (subItemContent == colcast(colID::ELU_COL)) {
			//replace comment
			bookmark_t rec = g_store.GetItem(item);

			rec.comment = pfc::string8(val);
			g_store.SetItem(item, rec);  //includes write data file

			g_store.Write();
		}
	}

	bool ILOD_BookmarkSource::listAllowTypeFindHere(ctx_t ctx, size_t item, size_t subItem) {
		CListControlBookmark* plc = (CListControlBookmark*)(ctx);
		auto subItemContent = plc->GetColContent(subItem);
		return subItemContent == colcast(colID::DESC_COL) ||
			subItemContent == colcast(colID::PLAYLIST_COL) ||
			subItemContent == colcast(colID::ELU_COL);
	}

	LRESULT CListControlBookmark::OnKeyDown(UINT, WPARAM p_wp, LPARAM, BOOL& bHandled) {

		switch (p_wp) {
		case VK_F2: {

				auto isingle = GetSingleSel();
				if (isingle != SIZE_MAX) {

					if (!TableEdit_IsActive()) {

						TableEdit_Start(isingle, colcast(colID::DESC_COL));

					}
				}
				return 0;
		}
		default:
			break;
		}

		bHandled = FALSE;
		return 0;
	}

	bool are_other_column_sorted(const CHeaderCtrl* hc, size_t current_col) {
		bool bres = false;

		HDITEM hditem;
		hditem.mask = HDI_FORMAT | HDI_ORDER;
		hditem.fmt |= HDF_OWNERDRAW;

		for (auto w = 0; w < hc->GetItemCount(); w++) {
			if (w == current_col) continue;
			auto bitems = hc->GetItem(w, &hditem);
			bool asc = hditem.fmt & HDF_SORTUP;
			bool desc = hditem.fmt & HDF_SORTDOWN;
			if (asc || desc) {
				return true;
			}
		}
		return bres;
	}

	inline static int field_compare(const char* p1, const char* p2) {
		return stricmp_utf8(p1, p2);
	}

	inline static int field_compare_rev(const char* p1, const char* p2) {
		return stricmp_utf8(p2, p2);
	}

	const pfc::string8& get_rec_col_content(const bookmark_t& rec, size_t col_content_index) {

		if (col_content_index == colcast(colID::DESC_COL)) {
			return rec.desc;
		}
		else if (col_content_index == colcast(colID::PLAYLIST_COL)) {
			return rec.playlist;
		}
		else if (col_content_index == colcast(colID::ELU_COL)) {
			return rec.comment;
		}
		else if (col_content_index == colcast(colID::DATE_COL)) {
			return rec.runtime_date;
		}
		else {
			PFC_ASSERT(false);
		}
		return rec.desc;
	}

	void get_sort_expr(const bookmark_t& bm, size_t colcontent, pfc::string8 & out) {

		if (colcontent == colcast(colID::TIME_COL)) {
			out = std::to_string(bm.get_time()).c_str();
		}
		else {
			out = get_rec_col_content(bm, colcontent);
		}
		if (colcontent != colcast(colID::DATE_COL)) {
			out << bm.runtime_date;
		}
		if (colcontent != colcast(colID::TIME_COL)) {
			out << std::to_string(bm.get_time()).c_str();
		}
		if (colcontent != colcast(colID::DESC_COL)) {
			out << bm.desc;
		}
		if (colcontent != colcast(colID::PLAYLIST_COL)) {
			out << bm.playlist;
		}

	}

	size_t calc_ordered_focus(size_t ifocus, size_t* order, size_t count) {

		if (ifocus != SIZE_MAX) {
			
			pfc::array_t<bool> focus_arr; focus_arr.set_size(count);
			pfc::fill_array_t(focus_arr, 0);
			focus_arr[ifocus] = true;
			pfc::reorder_t(focus_arr, order, count);

			for (auto n = 0; n < count; n++)
			{
				if (focus_arr[n]) {
					return n;
				}
			}
		}
		return SIZE_MAX;
	}

	void CListControlBookmark::OnColumnHeaderClick(t_size index) {

		auto colContentIndex = GetColContent(index);

		int header_ndx = static_cast<int>(index);

		if (colContentIndex == 0) {

			// #

			m_sorted_dir = !m_sorted_dir;

			//selection
			auto selMask = GetSelectionMask();
			auto csel = GetSelectedCount();
			auto ifocus = GetFocusItem();
			//

			HDITEM hditem;
			hditem.mask = HDI_FORMAT | HDI_ORDER;
			hditem.fmt |= HDF_OWNERDRAW;

			auto bitems = GetHeaderCtrl().GetItem(header_ndx, &hditem);

			bool asc = hditem.fmt & HDF_SORTUP;
			bool desc = hditem.fmt & HDF_SORTDOWN;

			hditem.fmt &= ~HDF_SORTDOWN;
			hditem.fmt &= ~HDF_SORTUP;

			bitems = GetHeaderCtrl().SetItem(header_ndx, &hditem);

			SetColumnSort(index, m_sorted_dir);

			bitems = GetHeaderCtrl().GetItem(header_ndx, &hditem);

			asc = hditem.fmt & HDF_SORTUP;
			desc = hditem.fmt & HDF_SORTDOWN;

			m_host->listColumnHeaderClick(this, index);
			ReloadItems(bit_array_true());

			if (auto cItems = GetItemCount()) {

				bit_array_bittable revMask(bit_array_false(), cItems);
				auto n = selMask.find_first(true, 0, cItems);
				for (n; n < cItems; n = selMask.find_next(true, n, cItems)) {
					revMask.set(cItems - 1 - n, true);
				}
				SetSelection(bit_array_true(), revMask);
				if (ifocus < cItems) {
					SetFocusItem(cItems - 1 - ifocus);
				}
				EnsureItemVisible(0, false);
			}
		}

		else if (colContentIndex > colcast(colID::ITEM_NUMBER)
			&& colContentIndex <= colcast(colID::DATE_COL)) {

			//time, desc, playlist, elu, date

			//todo
			if (GetSortOrder()) {
				SetSortOrder(false);
				if (auto count = GetItemCount()) {
					SelectNone();
					auto ifocus = GetFocusItem();
					SetFocusItem(count - 1);
					OnFocusChanged(ifocus, count - 1);
				}
			}

			auto tmpList = g_store.GetMasterList();

			HDITEM hditem;
			hditem.mask = HDI_FORMAT;

			auto items = GetHeaderCtrl().GetItem(header_ndx, &hditem);
			bool bheader_asc = hditem.fmt & HDF_SORTUP;
			bool bheader_desc = hditem.fmt & HDF_SORTDOWN;

			bool bsort_active = bheader_asc || bheader_desc;
			bool breverse = bheader_desc;

			if (!bsort_active) {

				bool bsome_col_sorted = are_other_column_sorted(&GetHeaderCtrl(), index/*current*/);

				if (bsome_col_sorted) {
					//todo
				}

				breverse = false;
			}
			else {
				breverse = !breverse;
			}

			SetColumnSort(index, !breverse);

			//selection

			pfc::list_t<pfc::string8> permuList; permuList.set_size(tmpList.size());

			for (auto w = 0; w < permuList.get_size(); w++) {
				get_sort_expr(tmpList[w], colContentIndex, permuList[w]);
			}

			pfc::array_t<size_t> order; order.resize(tmpList.size());
			order_helper::g_fill(order);

			//sel permutation
			//todo: sort_stable_get_permutation_t
			if (!breverse) {
				permuList.sort_get_permutation_t(path_compare, order.get_ptr());
			}
			else {
				permuList.sort_get_permutation_t(path_compare_rev, order.get_ptr());
			}

			//sort sel

			pfc::array_t<bool> sel_arr;
			selarr.append_fromptr(GetSelectionArray(), tmpList.size());
			pfc::reorder_t(sel_arr, order.get_ptr(), tmpList.size());

			bit_array_bittable new_sel(bit_array_false(), sel_arr.get_count());

			for (size_t walk = 0; walk < selarr.get_count(); ++walk) {
				new_sel.set(walk, sel_arr[walk]);
			}

			SetSelection(bit_array_true(), new_sel);

			//sort focus

			size_t ifocus = GetFocusItem();

			if (ifocus != SIZE_MAX) {
				auto count = tmpList.size();

				if ((ifocus = calc_ordered_focus(ifocus, order.get_ptr(), count)) != SIZE_MAX) {
					SetFocusItem(ifocus);
				}
				else {
					SetFocusItem(0);
				}
			}

			//todo: mod master list type, stable sort
			//sort list

			std::/*stable_*/sort(tmpList.begin(), tmpList.end(),
				[&](bookmark_t& abm, bookmark_t& bbm) {

					pfc::string8 a_str;
					pfc::string8 b_str;

					get_sort_expr(abm, colContentIndex, a_str);
					get_sort_expr(bbm, colContentIndex, b_str);

					std::string a = a_str.c_str();
					std::string b = b_str.c_str();

					if (!breverse) {
						return a < b;
					}
					else {
						return b < a;
					}

				});


			g_store.SetMasterList(std::move(tmpList));
			ReloadItems(bit_array_true());

		}
		else {

			//..
			return;
			//..

		}
	}
}