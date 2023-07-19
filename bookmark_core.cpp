#include "stdafx.h"

#include "bookmark_core.h"
#include "bookmark_list_dialog.h"

use namespace dlg;

namespace {


} //anonymous namespace

//==================Hooks for main menu=======================
void bbookmarkHook_store() { CListControlBookmarkDialog::storeBookmark(); }
void bbookmarkHook_restore() { CListControlBookmarkDialog::restoreFocusedBookmark(); }
void bbookmarkHook_clear() { CListControlBookmarkDialog::clearBookmarks(); }
