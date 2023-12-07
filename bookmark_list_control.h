#pragma once
#include <atlsafe.h>
#include "pfc/array.h"
#include "pfc/string-conv-lite.h"
#include "libPPUI/CListControlOwnerData.h"
#include "helpers/CListControlFb2kColors.h"

#include "IBookmark.h"
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
		bool listAllowTypeFind(ctx_t) override { return true; }
		// Allow type-find in a specific item/column?
		bool listAllowTypeFindHere(ctx_t ctx, size_t item, size_t subItem) override;

	public:
		//..
	};

	// wstring to BSTR wrapper
	inline CComBSTR ToBstr(const std::wstring& s)
	{
		if (s.empty())
		{
			return CComBSTR();
		}
		return CComBSTR(static_cast<int>(s.size()), s.data());
	}

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

		inline void AddFormaDeco(IDataObject* piifDataObject) {

			FORMATETC fmtetc = { 0 };
			fmtetc.cfFormat = CF_TEXT;
			fmtetc.dwAspect = DVASPECT_CONTENT;
			fmtetc.lindex = -1;
			fmtetc.tymed = TYMED_HGLOBAL;

			STGMEDIUM medium = { 0 };
			medium.tymed = TYMED_HGLOBAL;

			std::wstring wdeco_name;
			ibom::IBom::GetDecoClassName(wdeco_name);

			pfc::stringcvt::string_utf8_from_wide cnv_w;
			cnv_w.convert(wdeco_name.data());

			medium.hGlobal = GlobalAlloc(GMEM_MOVEABLE + GMEM_DDESHARE, cnv_w.length() + 1);
			if (medium.hGlobal) {
				char* pMem = (char*)GlobalLock(medium.hGlobal);
				if (pMem) {
					strcpy_s(pMem, cnv_w.length() + 1, cnv_w);
					HRESULT hr = GlobalUnlock(medium.hGlobal); // todo: check !!! ERROR
					hr = piifDataObject->SetData(&fmtetc, &medium, TRUE);
				}
			}
		}

		virtual pfc::com_ptr_t<IDataObject> MakeDataObject() override {

			const bool bUseDeco = true;

			metadb_handle_list mhl;
			bit_array_bittable selmask = GetSelectionMask();
			auto csel = GetSelectedCount();
			t_size selsize = selmask.size();

			if (m_sorted_dir) {
				bit_array_bittable tmpMask(selmask);
				for (size_t i = 0; i < selsize; i++) {
					selmask.set(selsize - 1 - i, tmpMask.get(i));
				}
			}

			pfc::stringcvt::string_wide_from_utf8_t cnv_w;
			std::vector<std::wstring > vselguids(GetSelectedCount());

			t_size walk = selmask.find_first(true, 0, selsize);
			for (walk; walk < selsize; walk = selmask.find_next(true, walk, selsize)) {

				// check paths
				const bookmark_t rec = glb::g_store.GetItem(walk);
				if (rec.path.get_length() && !rec.path.startsWith("https://")) {
					abort_callback_impl p_abort;
					try {
						if (!filesystem_v3::g_exists(rec.path.c_str(), p_abort)) {
							FB2K_console_print_e("Create D&D Bookmark failed...object not found.");
							continue;
						}
					}
					catch (exception_aborted) {
						//todo: log error/return
						break;
					}
				}

				auto metadb_ptr = metadb::get();
				metadb_handle_ptr track_bm = metadb_ptr->handle_create(rec.path.c_str(), rec.subsong);

				if (bUseDeco) {
					cnv_w.convert(pfc::print_guid(rec.guid_playlist));
					vselguids[mhl.get_count()] = (std::move(cnv_w));
				}

				mhl.add_item(track_bm);
			}

			if (m_sorted_dir) {
				pfc::array_t<t_size> order;
				order.resize(mhl.get_count());
				for (unsigned walk = 0; walk < mhl.size(); walk++) order[walk] = mhl.size() - 1 - walk;
				mhl.reorder(order.get_ptr());
				if (bUseDeco) {
					std::reverse(vselguids.begin(), vselguids.end());
				}
			}

			// mhl ready, make data object...

			static_api_ptr_t<playlist_incoming_item_filter> piif;

			if (bUseDeco) {

				// fb2k piif

				IDataObject* piifDataObject = piif->create_dataobject(mhl);
				AddFormaDeco(piifDataObject); // + TYMED_HGLOBAL CF_CHAR deco signature

				pfc::com_ptr_t<ibom::IBom> pBookmarkDataObject;

				// IDataObject cast

				pBookmarkDataObject.attach((ibom::IBom*)piifDataObject);

				if (!pBookmarkDataObject.is_valid()) {
					//todo: log error, continue without deco
					return piif->create_dataobject_ex(mhl);
				}

				// Bookmark container ready

				pBookmarkDataObject->SetPlaylistGuids(vselguids);
				return pBookmarkDataObject;  /*pfc::com_ptr_t<ibom::IBom>*/

			}
			else {

				// fb2k piif
				pfc::com_ptr_t<IDataObject> pDataObject = piif->create_dataobject_ex(mhl);
				return pDataObject;
			}

		}

		bool GetSortOrder() const { return m_sorted_dir; }
		void SetSortOrder(bool enable) { m_sorted_dir = enable; }
		void GetSortOrdererMask(bit_array_bittable& ordered_mask) {
			bit_array_bittable tmp_mask(ordered_mask);
			for (size_t w = 0; w < ordered_mask.size(); w++) ordered_mask.set(w, tmp_mask[tmp_mask.size() - 1 - w]);
		}

		size_t GetColContent(size_t icol) {

			return (*m_pcols_content)[icol];
		}

	protected:

		virtual void OnColumnHeaderClick(t_size index) override {
			if (GetColContent(index) != 0) {
				return;
			}

			m_sorted_dir = !m_sorted_dir;

			SelectNone();
			SetColumnSort(index, m_sorted_dir);

			m_host->listColumnHeaderClick(this, index);
			ReloadItems(bit_array_true());
			if (GetItemCount()) {
				SetFocusItem(0);
			}
		}

	private:

		bool m_sorted_dir = 0;
		bool m_cui = false;

		pfc::array_t<size_t>* m_pcols_content;

	};
}