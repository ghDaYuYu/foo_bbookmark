#include "bookmark_core.h"
#include "bookmark_automatic.h"
#include "bookmark_play_callback.h"

using namespace glb;

namespace {

	// stop

	void bm_play_callback::on_playback_stop(play_control::t_stop_reason p_reason) {

		if (!cfg_monitor.get()) {
			return;
		}

		if (p_reason == play_control::stop_reason_shutting_down) {
			if (cfg_autosave_on_quit.get()) {
				glb::g_bmAuto.updateDummy();
			}
		}
		else {
			g_bmAuto.resetDummyAll();
			glb::g_bmAuto.updateDummy();
		}
	}

	// new track

	void bm_play_callback::on_playback_new_track(metadb_handle_ptr p_track) {

		if (!cfg_monitor.get()) {
			return;
		}

		if (g_bmAuto.getDyna()) {
			FB2K_console_print_v("New dyna track arrived...");
		}
		else {
			FB2K_console_print_v("New track event...");
		}

		if (g_wnd_bookmark_pref) {
			SendMessage(g_wnd_bookmark_pref, UMSG_NEW_TRACK, NULL, NULL);
		}

		auto bm = g_bmAuto.getDummy();

		if (g_bmAuto.getDyna()) {
			g_bmAuto.resetDummyKeepDyna();
		}
		else {
			g_bmAuto.resetDummyAll();
		}


		glb::g_bmAuto.updateDummy();

		g_bmAuto.setDyna(false);

		auto nt = g_bmAuto.getDummy();
		if (!nt.need_playlist) {
			pfc::string8 q_orphan = !nt.playlist.get_length() ? " (no playlist)" : "";
			FB2K_console_print_v("New track details... ", nt.desc, q_orphan );
		}
		else {
			return;
		}

		bool bcan_autosave_newtrack = cfg_autosave_newtrack.get() && (!g_bmAuto.checkDummyIsRadio() || cfg_autosave_radio_newtrack.get());

		if (bcan_autosave_newtrack) {
			if (g_bmAuto.upgradeDummy(g_guiLists)) {
				if (is_cfg_Bookmarking()) {
					g_store.Write();
					bool bscroll_list = cfg_autosave_focus_newtrack.get();
					g_bmAuto.refresh_ui(bscroll_list, bscroll_list, g_store.GetMasterList(), g_guiLists);
				}
			}
			else {
				//..
			}
		}
	}

	// seek

	void bm_play_callback::on_playback_seek(double p_time) {

		if (!cfg_monitor.get()) {
			return;
		}


		g_bmAuto.updateDummyTime();
	}

	// time

	void bm_play_callback::on_playback_time(double p_time) {

		if (!cfg_monitor.get()) {
			return;
		}

		g_bmAuto.updateDummyTime();
	}

	void bm_play_callback::on_playback_dynamic_info_track(const file_info& p_info) {

		if (!cfg_monitor.get()) {
			return;
		}

		FB2K_console_print_v("Dynamic info arrived...");

		if (!g_bmAuto.checkDummyIsRadio()) {
			return;
		}

		{
			auto playback_control_ptr = playback_control::get();

			metadb_handle_ptr track_current;
			//Identify current track
			bool bnowPlaying = playback_control_ptr->get_now_playing(track_current);
			if (bnowPlaying) {
				g_bmAuto.setDyna(true);

				//

				on_playback_new_track(track_current);

				//
			}
		}
	}
}

// S T A T I C   P L A Y - C A L L B A C K

static service_factory_single_t<bm_play_callback> g_play_callback_static_factory;
