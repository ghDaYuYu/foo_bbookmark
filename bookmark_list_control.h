#pragma once

#include "pfc/array.h"

#include "libPPUI/CListControlOwnerData.h"
#include "helpers/CListControlFb2kColors.h"

#include "bookmark_core.h"

namespace dlg {

	const enum colID {
		ITEM_NUMBER = 0,
		TIME_COL,
		DESC_COL,
		PLAYLIST_COL,
		ELU_COL,
		DATE_COL,
		N_COLUMNS //6
	};

	class CListControlBookmark;

	class ILOD_BookmarkSource : public IListControlOwnerDataSource {

	public:

		//overrides...

		virtual size_t listGetItemCount(ctx_t) override;
		virtual pfc::string8 listGetSubItemText(ctx_t, size_t item, size_t subItem) override;
		bool listCanReorderItems(ctx_t) override { return true; }
		bool listReorderItems(ctx_t, const size_t*, size_t) override;
		bool listRemoveItems(ctx_t, pfc::bit_array const&) override;
		void listItemAction(ctx_t, size_t) override;
		bool listIsColumnEditable(ctx_t, size_t) override;
		void listSubItemClicked(ctx_t, size_t, size_t) override;
		uint32_t listGetEditFlags(ctx_t ctx, size_t item, size_t subItem) override;
		pfc::string8 listGetEditField(ctx_t ctx, size_t item, size_t subItem, size_t& lineCount) override;
		void listSetEditField(ctx_t ctx, size_t item, size_t subItem, const char* val) override;
		virtual void listColumnHeaderClick(ctx_t, size_t subItem) override {
			//..
		}

		// Called prior to a typefind pass attempt, you can either deny entirely, or prepare any necessary data and allow it.
		bool listAllowTypeFind(ctx_t) override { return false; }
		// Allow type-find in a specific item/column?
		bool listAllowTypeFindHere(ctx_t, size_t item, size_t subItem) override { return false; }
	
	public:
		//..
	};

	typedef CListControlFb2kColors <CListControlOwnerData> CListControlOwnerColors;

	class CListControlBookmark : public CListControlOwnerColors {

	public:

		CListControlBookmark(ILOD_BookmarkSource* h, bool is_cui) : CListControlOwnerColors(h), m_cui(is_cui) {
			//..
		}

		~CListControlBookmark() {

			//..
		}
		typedef CListControlOwnerColors TParent;

		BEGIN_MSG_MAP_EX(CListControlBookmark)
			CHAIN_MSG_MAP(TParent)
		END_MSG_MAP()
		void Initialize(pfc::array_t<size_t>* pcols_content) {
			InitializeHeaderCtrl(HDS_BUTTONS);
			m_pcols_content = pcols_content;
			SetPlaylistStyle();
			SetWantReturn(true);
			::SetWindowLongPtr(m_hWnd, GWL_EXSTYLE, 0);
		}

		virtual uint32_t QueryDragDropTypes() const override {
			return dragDrop_reorder | dragDrop_external;
		}

		virtual DWORD DragDropSourceEffects() override {
			return DROPEFFECT_MOVE | DROPEFFECT_COPY;
		}

		virtual pfc::com_ptr_t<IDataObject> MakeDataObject() override {

			static_api_ptr_t<playlist_incoming_item_filter> piif;

			metadb_handle_list mhl;
			bit_array_bittable selmask = GetSelectionMask();
			t_size selsize = selmask.size();

			for (t_size walk = selmask.find_first(true, 0, selsize); walk < selsize;
				walk = selmask.find_next(true, walk, selsize)) {

				auto rec = glb::g_masterList[walk];
				if (!rec.path.startsWith("https://")) {
					abort_callback_impl p_abort;
					try {
						if (!filesystem_v3::g_exists(rec.path.c_str(), p_abort)) {
							FB2K_console_print_e("Create D&D Bookmark failed...object not found.");
							continue;
						}
					}
					catch (exception_aborted) {
						break;
					}
				}

				auto metadb_ptr = metadb::get();
				metadb_handle_ptr track_bm = metadb_ptr->handle_create(rec.path.c_str(), rec.subsong);
				mhl.add_item(track_bm);
			}

			pfc::com_ptr_t<IDataObject> pDataObject = piif->create_dataobject_ex(mhl);
			return pDataObject;
		}

		bool GetSortOrder() const { return m_sorted_dir; }
		void SetSortOrder(bool enable) { m_sorted_dir = enable; }

		size_t GetColContent(size_t icol) {
			return (*m_pcols_content)[icol];
		}

	protected:

		virtual void OnColumnHeaderClick(t_size index) override {
			if (index) {
				return;
			}
			m_sorted_dir = !m_sorted_dir;
			SelectNone();
			SetColumnSort(index, m_sorted_dir);
			m_host->listColumnHeaderClick(this, index);
			ReloadItems(bit_array_true());
			if (GetItemCount()) {
				auto fi = GetFocusItem();
				if (fi != pfc_infinite) {
					SetFocusItem(m_sorted_dir ? fi - (GetItemCount() - 1) : GetItemCount() - 1 - fi);
				}
			}
		}

	private:

		bool m_sorted_dir = 0;
		bool m_cui = false;

		pfc::array_t<size_t>* m_pcols_content;

	};
}