#pragma once

#include "SDK/play_callback.h"

namespace {

	class bm_play_callback : public play_callback_static {

	public:

		// overrides 

		void on_playback_starting(play_control::t_track_command p_command, bool p_paused) override {}
		void on_playback_pause(bool p_state) override {}
		void on_playback_edited(metadb_handle_ptr p_track) override {}
		void on_playback_dynamic_info(const file_info& p_info) override {}
		void on_playback_dynamic_info_track(const file_info& p_info) override {}
		void on_volume_change(float p_new_val) override {}

		void on_playback_stop(play_control::t_stop_reason p_reason) override;
		void on_playback_new_track(metadb_handle_ptr p_track) override;
		void on_playback_seek(double p_time) override;
		void on_playback_time(double p_time) override;

		// select callbacks...

		virtual unsigned get_flags() {

			return flag_on_playback_new_track | flag_on_playback_seek | flag_on_playback_time | flag_on_playback_stop;
		}
	};
}
