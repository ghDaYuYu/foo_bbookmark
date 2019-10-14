#include "stdafx.h"
#include "bookmark_persistence.h"
#include "bookmark_preferences.h"
#include "bookmark_worker.h"


double g_pendingSeek;

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
		console::formatter() << "get_now_playing failed, can only store time.";
		songDesc << "Could not find song info.";

		newMark.m_time = playback_control_ptr->playback_get_position();
		newMark.m_desc = songDesc;
		newMark.m_playlist = "";	//without using c_str(), the full 80 characters are written every time
		newMark.m_path = "";
		newMark.m_subsong = 0;
	}
	else {
		titleformat_object::ptr desc_format;
		static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(desc_format, cfg_bookmark_desc_format.c_str());

		if (!dbHandle_item->format_title(NULL, songDesc, desc_format, NULL)) {
			songDesc << "Could not generate Description.";
		}

		//TODO: graceful failure?!
		pfc::string_fixed_t<80> playing_pl_name = "Could not read playlist name.";
		size_t index_playlist;
		size_t index_item;
		auto playlist_manager_ptr = playlist_manager::get();
		if (playlist_manager_ptr->get_playing_item_location(&index_playlist, &index_item))
			playlist_manager_ptr->playlist_get_name(index_playlist, playing_pl_name);

		pfc::string songPath = dbHandle_item->get_path();

		newMark.m_time = playback_control_ptr->playback_get_position();
		newMark.m_desc = songDesc;
		newMark.m_playlist = playing_pl_name.c_str();	//without using c_str(), the full 80 characters are written every time
		newMark.m_path = songPath;
		newMark.m_subsong = dbHandle_item->get_subsong_index();
	}


	masterList.emplace_back(newMark);
}

void bookmark_worker::restore(std::vector<bookmark_t>& masterList, size_t index) {
	if (masterList.empty()) {	//Nothing to restore
		console::complain("Restore Bookmark failed", "No bookmark.");
		return;
	}

	if (index >= 0 && index < masterList.size()) {	//load using the index
		auto rec = masterList[index];
		g_pendingSeek = 0.0;	//reset just in case

		//restore track:
		auto metadb_ptr = metadb::get();
		auto playlist_manager_ptr = playlist_manager::get(); //Get index of stored playlist
		auto playback_control_ptr = playback_control::get();

		metadb_handle_ptr track_bm = metadb_ptr->handle_create(rec.m_path.c_str(), rec.m_subsong);	//Identify track to restore

		metadb_handle_ptr track_current;
		playback_control_ptr->get_now_playing(track_current); //Identify current track

		if (track_bm != track_current) { //Skip if track is already playing					
			size_t index_pl = playlist_manager_ptr->find_playlist(rec.m_playlist.c_str()); //TODO: Also check this initially

			if (index_pl == pfc_infinite) {	//Complain if no playlist of that name exists anymore 
				console::complain("Bookmark Restoration partially failed", "Could not find Playlist");
			}
			else {
				size_t index_item;
				if (!playlist_manager_ptr->playlist_find_item(index_pl, track_bm, index_item)) {	//Complain if the track does not exist in that playlist
					console::complain("Bookmark Restoration partially failed", "Could not find Track");
				}
				else {
					//Change track by way of the queue:
					playlist_manager_ptr->queue_flush();
					playlist_manager_ptr->queue_add_item_playlist(index_pl, index_item);

					playback_control_ptr->next();
					g_pendingSeek = rec.m_time; //This will be applied by bmWorker_play_callback::on_playback_new_track once the change of track has gone through
				}
			}
		}

		if (g_pendingSeek == 0.0) { //If a time change was not queued up, the track is either already correct or could not be determined
			if (!core_api::assert_main_thread()) console::formatter() << "not the m thread";
			console::formatter() << "Restoring time:" << rec.m_time;
			playback_control_ptr->playback_seek(rec.m_time);
		}


		//unpause
		playback_control_ptr->pause(false);
	}
	else {	//Index invalid, fall back to the 0th entry
		restore(masterList, 0);
		return;
	}
}



//---Play Callback---
class bmWorker_play_callback : public play_callback_static {
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
	//(by scheduling it in the main thread).
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

	/* The play_callback_manager enumerates play_callback_static services and registers them automatically. We only have to provide the flags indicating which callbacks we want. */
	virtual unsigned get_flags() {
		return flag_on_playback_new_track;
	}
};

static service_factory_single_t<bmWorker_play_callback> g_play_callback_static_factory;
