#include "bookmark_core.h"

namespace {

	class initquit_bbookmark : public initquit {

		virtual void on_init() {

			g_permStore.readDataFile(g_masterList);
			//note: on_init runs after main window creation
			g_primaryGuiList->ReloadData();
		}

		virtual void on_quit() {

			if (cfg_autosave_on_quit) {
				if (g_bmAuto.upgradeDummy(g_masterList, g_guiLists)) {
					g_permStore.writeDataFile(g_masterList);
				}
			}
		}
	};

	static initquit_factory_t< initquit_bbookmark > g_bookmark_initquit;
}