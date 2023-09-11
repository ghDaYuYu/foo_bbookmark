#include "bookmark_core.h"
#include "bookmark_automatic.h"
#include "bookmark_play_callback.h"

using namespace glb;

namespace {

	// stop

	void bm_play_callback::on_playback_stop(play_control::t_stop_reason p_reason) {

		if (!core_api::is_shutting_down() && cfg_autosave_on_quit.get()) {

			g_bmAuto.updateDummy();
		}
	}

	// new track

	void bm_play_callback::on_playback_new_track(metadb_handle_ptr p_track) {

		if (g_wnd_bookmark_pref) {
			SendMessage(g_wnd_bookmark_pref, UMSG_NEW_TRACK, NULL, NULL);
		}

		if (cfg_autosave_newtrack.get() || cfg_autosave_on_quit.get()) {

			if (!g_bmAuto.checkDummy()) {
				g_bmAuto.updateDummy();
			}

			if (cfg_autosave_newtrack.get()) {
				{
					std::lock_guard<std::mutex> ul(g_mtx_restoring);
					if (!g_restoring) {

						if (g_bmAuto.upgradeDummy(g_masterList, g_guiLists)) {

							if (is_cfg_Bookmarking()) {
								g_permStore.writeDataFile(g_masterList);
							}
						}
						else {
							//..
						}

					}
				}
			}
		}

		// EXIT

		{
			std::lock_guard<std::mutex> ul(g_mtx_restoring);
			if (g_restoring) {
				g_restoring = false;
				return;
			}
		}
	}

	// seek

	void bm_play_callback::on_playback_seek(double p_time) {

		g_bmAuto.updateDummyTime();
	}

	// time

	void bm_play_callback::on_playback_time(double p_time) {

		g_bmAuto.updateDummyTime();
	}
}

static service_factory_single_t<bm_play_callback> g_play_callback_static_factory;
