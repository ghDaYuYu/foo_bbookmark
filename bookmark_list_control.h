#pragma once

#include "pfc/array.h"

#include "libPPUI/CListControlOwnerData.h"
#include "helpers/CListControlFb2kColors.h"

#include "bookmark_core.h"

namespace dlg {

	const enum colID {
		TIME_COL = 0,
		DESC_COL,
		PLAYLIST_COL,
		ELU_COL,
		DATE_COL,
		N_COLUMNS
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

		void Initialize(HWND hparent, pfc::array_t<size_t>* pcolContent) {

			m_pcolContent = pcolContent;

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
				
				auto metadb_ptr = metadb::get();
				metadb_handle_ptr track_bm = metadb_ptr->handle_create(rec.path.c_str(), rec.subsong);
				mhl.add_item(track_bm);
			}

			pfc::com_ptr_t<IDataObject> pDataObject = piif->create_dataobject_ex(mhl);
			return pDataObject;
		}

		size_t GetColContent(size_t icol) {

			return (*m_pcolContent)[icol];
		}

	private:

		bool m_cui = false;

		pfc::array_t<size_t>* m_pcolContent;

	};
}