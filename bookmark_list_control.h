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

		bool listIsColumnEditable(ctx_t, size_t) override { return false; }

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

		void Initialize(HWND hparent, HFONT hdlgFont, pfc::array_t<size_t>* pcolContent) {

			m_pcolContent = pcolContent;

			SetPlaylistStyle();

			if (m_cui) {
			

				//TODO:

				
			}
			else {
				SetHeaderFont(hdlgFont);
				SetFont(hdlgFont);
				SetWantReturn(true);
			}
		}

		size_t GetColContent(size_t icol) {

			return (*m_pcolContent)[icol];
		}

	private:

		bool m_cui = false;

		pfc::array_t<size_t>* m_pcolContent;

	};
}