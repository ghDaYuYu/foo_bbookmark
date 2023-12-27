#include "bookmark_core.h"
#include "bookmark_list_control.h"

using namespace glb;

namespace {

	class initquit_bbookmark : public initquit {

		virtual void on_init() {

			g_store.Initialize();

			if (g_primaryGuiList) {
				g_primaryGuiList->ReloadData();
				g_primaryGuiList->RestoreLastFocus();
			}
		}

		virtual void on_quit() {

			if (is_cfg_Bookmarking() && cfg_autosave_on_quit.get() && g_bmAuto.checkDummy()) {
				g_bmAuto.upgradeDummy(g_guiLists);
			}

			g_store.Write();
		}
	};

	// S T A T I C   I N I T / Q U I T

	static initquit_factory_t< initquit_bbookmark > g_bookmark_initquit;
}