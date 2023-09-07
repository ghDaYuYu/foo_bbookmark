#include "stdafx.h"
#include "bookmark_core.h"
#include "bookmark_persistence.h"
#include "bookmark_preferences.h"
#include "bookmark_worker.h"


double g_pendingSeek;

using namespace glb;

bookmark_worker::bookmark_worker()
{
}


bookmark_worker::~bookmark_worker()
{
}

void bookmark_worker::store(std::vector<bookmark_t>& masterList) {
	pfc::string_formatter songDesc;
	bookmark_t newMark = bookmark_t();

	metadb_handle_ptr dbHandle_item;
	auto playback_control_ptr = playback_control::get();
	if (!playback_control_ptr->get_now_playing(dbHandle_item)) {
		//We can not obtain the currently playing item - fizzle out
		FB2K_console_print_e("Get_now_playing failed, can only store time.");
		songDesc << "Could not find playing song info.";

		newMark.time = playback_control_ptr->playback_get_position();
		newMark.desc = songDesc;
		newMark.playlist = "";
		newMark.guid_playlist = pfc::guid_null;
		newMark.path = "";
		newMark.subsong = 0;
		gimme_time(newMark.date);
	}
	else {
		titleformat_object::ptr desc_format;
		static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(desc_format, cfg_desc_format.get_value().c_str());

		if (!dbHandle_item->format_title(NULL, songDesc, desc_format, NULL)) {
			songDesc << "Could not generate Description.";
		}

		//TODO: graceful failure?!
		pfc::string_fixed_t<80> playing_pl_name = "Could not read playlist name.";
		size_t index_playlist;
		GUID guid_playlist = pfc::guid_null;
		size_t index_item;
		auto playlist_manager_ptr = playlist_manager_v5::get();
		if (playlist_manager_ptr->get_playing_item_location(&index_playlist, &index_item)) {
			playlist_manager_ptr->playlist_get_name(index_playlist, playing_pl_name);
			if (core_version_info_v2::get()->test_version(2, 0, 0, 0)) {
				guid_playlist = playlist_manager_v5::get()->playlist_get_guid(index_playlist);
			}
		}

		pfc::string8 songPath = dbHandle_item->get_path();

		newMark.time = playback_control_ptr->playback_get_position();
		newMark.desc = songDesc;
		newMark.playlist = playing_pl_name.c_str();
		newMark.guid_playlist = guid_playlist;
		newMark.path = songPath;
		newMark.subsong = dbHandle_item->get_subsong_index();
		gimme_time(newMark.date);
	}

	masterList.emplace_back(newMark);
}

void bookmark_worker::restore(std::vector<bookmark_t>& masterList, size_t index) {
	if (masterList.empty()) {	//Nothing to restore
		FB2K_console_print_v("Restore Bookmark failed...no bookmarks.");
		return;
	}

	if (index >= 0 && index < masterList.size()) {	//load using the index
		auto rec = masterList[index];

		if (!(bool)rec.path.get_length()) {
			FB2K_console_print_v("Restore Bookmark failed...no track in bookmark.");
			return;
		}

		g_pendingSeek = 0.0;	//reset just in case

		//restore track:
		auto metadb_ptr = metadb::get();
		auto playlist_manager_ptr = playlist_manager::get(); //Get index of stored playlist
		auto playback_control_ptr = playback_control::get();

		metadb_handle_ptr track_bm = metadb_ptr->handle_create(rec.path.c_str(), rec.subsong);	//Identify track to restore

		//if (track_bm != track_current) { //Skip if track is already playing
			size_t index_pl = ~0;
			if (core_version_info_v2::get()->test_version(2, 0, 0, 0)) {
				index_pl = playlist_manager_v5::get()->find_playlist_by_guid(rec.guid_playlist);
			}

			if (index_pl == pfc_infinite) {	//Complain if no playlist of that name exists anymore 
				FB2K_console_print_v("Bookmark Restoration partially failed", "Could not find Playlist '", rec.playlist, "'");

				if (track_bm.get_ptr()) {
					//Change track by way of the queue:
					playlist_manager_ptr->queue_flush();

					{
						std::lock_guard<std::mutex> ul(g_mtx_restoring);
						g_restoring = true;
					}

					playlist_manager_ptr->queue_add_item(track_bm);
					playback_control_ptr->next();
					g_pendingSeek = rec.time;
				}
			}
			else {
				size_t index_item = ~0;

				if (index_pl == ~0 || !playlist_manager_ptr->playlist_find_item(index_pl, track_bm, index_item)) {	//Complain if the track does not exist in that playlist
					FB2K_console_print_v("Bookmark Restoration partially failed", "Could not find Track '", track_bm->get_path(), "'");
				}
				else {

					if (is_cfg_Queuing()) {
						//Change track by way of the queue:
						playlist_manager_ptr->queue_flush();

						{
							std::lock_guard<std::mutex> ul(g_mtx_restoring);
							g_restoring = true;
						}

						playlist_manager_ptr->queue_add_item_playlist(index_pl, index_item);

						playback_control_ptr->next();
					}
					else {
						size_t plpos;
						playlist_manager_ptr->playlist_find_item(index_pl, track_bm, plpos);
						if (plpos != ~0) {
							playlist_manager_ptr->set_active_playlist(index_pl);
							playlist_manager_ptr->set_playing_playlist(index_pl);
							playlist_manager_ptr->playlist_set_selection(index_pl, bit_array_true(), bit_array_false());
							playlist_manager_ptr->playlist_set_selection_single(index_pl, plpos, true);
							playlist_manager_ptr->playlist_set_focus_item(index_pl, plpos);

							{
								std::lock_guard<std::mutex> ul(g_mtx_restoring);
								g_restoring = true;
							}
							playlist_manager_ptr->playlist_execute_default_action(index_pl, plpos);
						}
					}
					//This will be applied by the worker play callback
					g_pendingSeek = rec.time;
				}
			}
		//} end track already playing

		{
			std::lock_guard<std::mutex> ul(g_mtx_restoring);
			if (g_restoring == true) {
			
				if (g_pendingSeek == 0.0) { //If a time change was not queued up, the track is either already correct or could not be determined
					if (!core_api::assert_main_thread()) {
						FB2K_console_print_v("(Not in main thread)");
					}
					FB2K_console_print_v("Restoring time:", rec.time);
					playback_control_ptr->playback_seek(rec.time);
				}

				//unpause
				playback_control_ptr->pause(false);

			}
		}
	}
	else {	//Index invalid, fall back to the last entry
		restore(masterList, masterList.size() - 1);
		return;
	}
}


// worker play callback

class bm_worker_play_callback : public play_callback_static {
public:
	/* play_callback methods go here */
	void on_playback_starting(play_control::t_track_command p_command, bool p_paused) override {}
	void on_playback_stop(play_control::t_stop_reason p_reason) override {}
	void on_playback_pause(bool p_state) override {}
	void on_playback_edited(metadb_handle_ptr p_track) override {}
	void on_playback_dynamic_info(const file_info & p_info) override {}
	void on_playback_dynamic_info_track(const file_info & p_info) override {}
	void on_volume_change(float p_new_val) override {}
	void on_playback_seek(double p_time) override {}
	void on_playback_time(double p_time) override {}

	//To apply the seek to the right track, we need to wait for it to start playing;
	//and to prevent race conditions we need to get the actual seeking out of the on_playback
	//(by scheduling it in the main thread)

	void on_playback_new_track(metadb_handle_ptr p_track) override {

		if (g_pendingSeek != 0.0) {

			//Lambda can't capture the global variable directly

			fb2k::inMainThread([d = g_pendingSeek] {
				if (playback_control::get()->playback_can_seek()) {
					playback_control::get()->playback_seek(d);
				}
			});

			g_pendingSeek = 0.0;

		}
	}

	// select worker play callbacks

	virtual unsigned get_flags() {

		return flag_on_playback_new_track;
	}
};

static service_factory_single_t<bm_worker_play_callback> g_worker_play_callback_static_factory;
