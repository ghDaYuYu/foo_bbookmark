#include "bookmark_core.h"

#include "bookmark_list_control.h"

using namespace glb;

namespace {

	class initquit_bbookmark : public initquit {

		virtual void on_init() {

			g_permStore.readDataFileJSON(g_masterList);

			if (g_primaryGuiList) {
				g_primaryGuiList->ReloadData();
			}
		}

		virtual void on_quit() {

			if (cfg_autosave_on_quit.get()) {

				if (g_bmAuto.upgradeDummy(g_masterList, g_guiLists)) {
					g_permStore.writeDataFileJSON(g_masterList);
				}

			}
		}
	};

	static initquit_factory_t< initquit_bbookmark > g_bookmark_initquit;
}