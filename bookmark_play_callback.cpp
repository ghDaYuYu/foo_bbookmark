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

			bool bcan_autosave_newtrack = cfg_autosave_newtrack.get() && (!g_bmAuto.checkDummyIsRadio() || cfg_autosave_radio_newtrack.get());

			if (bcan_autosave_newtrack) {

				std::lock_guard<std::mutex> ul(g_mtx_restoring);
				if (!g_restoring) {

					if (g_bmAuto.upgradeDummy(g_masterList, g_guiLists)) {

						if (is_cfg_Bookmarking()) {
							g_permStore.writeDataFile(g_masterList);
							bool bscroll_list = cfg_autosave_focus_newtrack.get();
							g_bmAuto.refresh_ui(bscroll_list, bscroll_list, g_masterList, g_guiLists);
						}

					}
					else {
						//..
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

	void bm_play_callback::on_playback_dynamic_info_track(const file_info& p_info) {

		if (!g_bmAuto.checkDummyIsRadio()) {
			return;
		}

		auto playback_control_ptr = playback_control::get();

		metadb_handle_ptr track_current;
		//Identify current track
		bool bnowPlaying = playback_control_ptr->get_now_playing(track_current);
		if (bnowPlaying) {
			on_playback_new_track(track_current);
		}

	}
}

// S T A T I C   P L A Y - C A L L B A C K

static service_factory_single_t<bm_play_callback> g_play_callback_static_factory;
