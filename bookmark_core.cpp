#include "stdafx.h"

#include "bookmark_core.h"
#include "bookmark_list_dialog.h"

using namespace dlg;

namespace {


} //anonymous namespace

//==================Hooks for main menu=======================

void bbookmarkHook_store() { CListCtrlMarkDialog::addBookmark(); }
void bbookmarkHook_restore() { CListCtrlMarkDialog::restoreFocusedBookmark(); }
void bbookmarkHook_clear() { CListCtrlMarkDialog::clearBookmarks(); }

bool bbookmarkHook_canStore() { return CListCtrlMarkDialog::canStore(); }
bool bbookmarkHook_canRestore() { return CListCtrlMarkDialog::canRestore(); }
bool bbookmarkHook_canClear() { return CListCtrlMarkDialog::canClear(); }
