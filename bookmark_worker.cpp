#include "stdafx.h"
#include "bookmark_core.h"
#include "bookmark_store.h"
#include "bookmark_persistence.h"
#include "bookmark_preferences.h"
#include "bookmark_worker.h"

double g_pendingSeek;

using namespace glb;

bookmark_worker::bookmark_worker()
{
	//..
}


bookmark_worker::~bookmark_worker()
{
	//..
}

void bookmark_worker::store(const bookmark_t bookmark) {

	bookmark_t newMark;

	if (cfg_monitor.get()) {
		newMark = bookmark;
	}
	else {
		pfc::string_formatter songDesc;
		metadb_handle_ptr dbHandle_item;
		auto playback_control_ptr = playback_control::get();

		if (!playback_control_ptr->get_now_playing(dbHandle_item)) {

			//We can NOT obtain the currently playing item - fizzle out
			FB2K_console_print_e("Get_now_playing failed, can only store time.");
			songDesc << "Could not find playing song info.";

			newMark.set_time(playback_control_ptr->playback_get_position());
			newMark.desc = songDesc.c_str();
			newMark.playlist = "";
			newMark.guid_playlist = pfc::guid_null;
			newMark.path = "";
			newMark.subsong = 0;
			gimme_date(newMark);
		}
		else {
			titleformat_object::ptr desc_format;
			static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(desc_format, cfg_desc_format.get_value().c_str());

			if (!dbHandle_item->format_title(NULL, songDesc, desc_format, NULL)) {
				songDesc << "Could not generate Description.";
			}

			//TODO: graceful failure?!
			pfc::string_fixed_t<80> playing_pl_name = cfg_monitor ? "Could not read playlist name." : "";
			size_t index_playlist;
			GUID guid_playlist = pfc::guid_null;
			size_t index_item;
			auto playlist_manager_ptr = playlist_manager_v5::get();
			if (playlist_manager_ptr->get_playing_item_location(&index_playlist, &index_item)) {
				playlist_manager_ptr->playlist_get_name(index_playlist, playing_pl_name);
				guid_playlist = playlist_manager_v5::get()->playlist_get_guid(index_playlist);
			}

			pfc::string8 songPath = dbHandle_item->get_path();

			newMark.set_time(playback_control_ptr->playback_get_position());
			newMark.desc = songDesc;
			newMark.playlist = playing_pl_name.c_str();	//without using c_str(), the full 80 characters are written every time
			newMark.guid_playlist = guid_playlist;
			newMark.path = songPath;
			newMark.subsong = dbHandle_item->get_subsong_index();
			gimme_date(newMark);

			if (newMark.isRadio()) {
				titleformat_object::ptr p_script;
				pfc::string8 titleformat = cfg_desc_format.get_value();
				static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(p_script, titleformat);

				pfc::string_formatter songDesc;
				if (playback_control::get()->playback_format_title(NULL, songDesc, p_script, NULL, playback_control::display_level_all)) {
					newMark.desc = songDesc.c_str();
				}
			}
		}
	}

	g_store.AddItem(newMark);
}

void bookmark_worker::restore(size_t index) {

	const std::vector<bookmark_t>& masterList = g_store.GetMasterList();
	bool bempty = g_store.GetMasterList().empty();

	if (masterList.empty() || index >= masterList.size()) {
		FB2K_console_print_v("Restore Bookmark failed... invalid position");
		return;
	}


	if (index >= 0 && index < masterList.size()) {	//load using the index
		auto rec = masterList[index];

		g_bmAuto.SetRestoredDummy(rec);

		if (!(bool)rec.path.get_length()) {
			FB2K_console_print_v("Restore Bookmark failed...no track in bookmark.");
			return;
		}

		if (!rec.isRadio()) {
			abort_callback_impl p_abort;
			try {
				if (!filesystem_v3::g_exists(rec.path.c_str(), p_abort)) {
					FB2K_console_print_e("Error restoring bookmark... object not found.");
					return;
				}
			}
			catch (exception_aborted) {
				return;
			}
		}

		//restore track:
		auto metadb_ptr = metadb::get();
		auto playlist_manager_ptr = playlist_manager::get(); //Get index of stored playlist
		auto playback_control_ptr = playback_control::get();

		metadb_handle_ptr track_bm = metadb_ptr->handle_create(rec.path.c_str(), rec.subsong);	//Identify track to restore

			size_t index_pl = ~0;
			size_t index_item = ~0;
			bool bplaylist_item_fault = true;
			
			if (pfc::guid_equal(rec.guid_playlist, pfc::guid_null)) {
				FB2K_console_print_v("Item queued... ", rec.desc);
			}
			else {
				index_pl = playlist_manager_v5::get()->find_playlist_by_guid(rec.guid_playlist);

				bplaylist_item_fault = index_pl == pfc_infinite || !playlist_manager_ptr->playlist_find_item(index_pl, track_bm, index_item);
				if (bplaylist_item_fault) {
					//Complain if no playlist of that name exists anymore or item not in playlist
					FB2K_console_print_v("Item queued... '", rec.playlist, "' playlist/item mismatch.");
				}
			}

			if (is_cfg_Queuing() || bplaylist_item_fault) {

				if (track_bm.get_ptr()) {
					if (is_cfg_Flush_Queue()) {
						playlist_manager_ptr->queue_flush();
					}

					g_pendingSeek = 0.0; //cancel queued item start time

					bool bplaying = playback_control_ptr->is_playing();
					bool bpaused = playback_control_ptr->is_paused();
					auto cq = playlist_manager_ptr->queue_get_count();

					//todo: if queue is modified after this, seek time will be applied to the wrong item ?

					if (!(bpaused || cq)) {
						//only the first item send to an empty queue can be seek
						g_pendingSeek = rec.get_time();
					}

					if (bplaylist_item_fault) {
						playlist_manager_ptr->queue_add_item(track_bm);
					}
					else {
						playlist_manager_ptr->queue_add_item_playlist(index_pl, index_item);
					}

					//todo: rev update dummy relies on new track event
					if (is_cfg_Queuing() && playlist_manager_ptr->queue_get_count() == 1 && !playback_control_ptr->is_playing()) {
						//..
					}
					else {
							if ((playlist_manager_ptr->queue_get_count() > 1) && playback_control_ptr->is_playing()) {
								return;
							}
					}

					playback_control_ptr->play_or_unpause();
				} 
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
					playlist_manager_ptr->playlist_execute_default_action(index_pl, plpos);
				}

				g_pendingSeek = rec.get_time();

				if (g_pendingSeek == 0.0) { //If a time change was not queued up, the track is either already correct or could not be determined
					if (!core_api::assert_main_thread()) {
						FB2K_console_print_v("(Not in main thread)");
					}

					FB2K_console_print_v("Restoring time:", rec.get_time());

					playback_control_ptr->playback_seek(rec.get_time());
				}

				//unpause
				playback_control_ptr->pause(false);
			}
		}
	}
	else {	//Index invalid, fall back to the last entry
		restore(masterList.size() - 1);
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
