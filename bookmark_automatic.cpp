#include "stdafx.h"

#include "SDK/playback_control.h"
#include "SDK/playlist.h"

//update playlist
#include "bookmark_core.h"

#include "bookmark_automatic.h"
#include "bookmark_list_control.h"
#include "bookmark_preferences.h"

void bookmark_automatic::updateDummyTime() {

	auto oldtime = dummy.time;
	dummy.time = playback_control::get()->playback_get_position();

	if (m_updatePlaylist) {
		m_updatePlaylist = false;

		pfc::string_fixed_t<80> playing_pl_name;
		size_t index_playlist;
		size_t index_item;
		auto playlist_manager_ptr = playlist_manager_v5::get();

		if (playlist_manager_ptr->get_playing_item_location(&index_playlist, &index_item)) {
			FB2K_console_print_v("AutoBookmark: dummy update: Playlist index is ", index_playlist);
			if (playlist_manager_ptr->playlist_get_name(index_playlist, playing_pl_name))
				FB2K_console_print_v("AutoBookmark: dummy update: Playlist name is ", playing_pl_name);
		}
		else {
			FB2K_console_print_v("AutoBookmark: dummy update: couldn't find playlist index");
		}

		//Set the flag back to true if either operation fails
		m_updatePlaylist |= !playlist_manager_ptr->get_playing_item_location(&index_playlist, &index_item);
		m_updatePlaylist |= !playlist_manager_ptr->playlist_get_name(index_playlist, playing_pl_name);

		dummy.playlist = playing_pl_name.c_str();

		if (core_version_info_v2::get()->test_version(2, 0, 0, 0)) {
			dummy.guid_playlist = playlist_manager_ptr->playlist_get_guid(index_playlist);
		}

		bool canBookmark = is_cfg_Bookmarking();
		canBookmark &= (cfg_autosave_newtrack.get() || (core_api::is_shutting_down() && cfg_autosave_on_quit.get()));
		canBookmark &= CheckAutoFilter();

		if (!m_updatePlaylist && canBookmark ) {
			//update last item
			//todo: delay insertion - add instead of update.
			auto cMastList = glb::g_masterList.size();
			bit_array_bittable changeMask(bit_array_false(), cMastList);
			
			auto& last = cMastList ? glb::g_masterList.at(cMastList - 1) : bookmark_t();
			if (!cMastList || last.playlist.get_length()) {
				//(list was empty/filtered playlist)
				//if the last bookmark's playlist was assigned, add new item
				dummy.time = oldtime;
				glb::g_masterList.emplace_back(dummy);
				changeMask.resize(glb::g_masterList.size());
			}
			else {
				//item was waiting for playlist details
				last.playlist = dummy.playlist;
				last.guid_playlist = dummy.guid_playlist;
			}
			changeMask.set(glb::g_masterList.size() - 1, true);
			for (auto gui : glb::g_guiLists) {
				gui->ReloadItems(changeMask);
			}
			//
		}

		gimme_time(dummy.date);
	}
}

//Fully update the dummy.
void bookmark_automatic::updateDummy() {

	metadb_handle_ptr dbHandle_item;
	auto playback_control_ptr = playback_control::get();

	if (playback_control_ptr->get_now_playing(dbHandle_item)) {
		pfc::string_formatter songDesc;
		titleformat_object::ptr desc_format;
		static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(desc_format, cfg_desc_format.get_value().c_str());

		if (!dbHandle_item->format_title(NULL, songDesc, desc_format, NULL)) {
			songDesc << "Could not generate Description.";
		}

		m_updatePlaylist = true;	//We can't read the PlName right after the track was changed
		dummy.playlist = "";
		dummy.guid_playlist = pfc::guid_null;

		pfc::string8 songPath = dbHandle_item->get_path();

		//TODO: graceful failure?!

		dummy.time = playback_control_ptr->playback_get_position();
		dummy.desc = songDesc;
		dummy.path = songPath;
		dummy.subsong = dbHandle_item->get_subsong_index();
		gimme_time(dummy.date);
	}
	else {
		dummy.time = 0.0;
		dummy.desc = "";
		dummy.path = "";
		dummy.subsong = 0;
	}
}

bool bookmark_automatic::CheckAutoFilter() {
	if (cfg_autosave_filter_newtrack.get()) {
		//filter is active

		//Obtain individual names in the filter
		std::vector<std::string> allowedPlaylists;
		std::stringstream ss(cfg_autosave_newtrack_playlists.get_value().c_str());
		std::string token;
		while (std::getline(ss, token, ',')) {
			allowedPlaylists.push_back(token);
		}

		//replace chars in the current playlist names
		pfc::string8 dummyPlaylist = dummy.playlist.c_str();
		dummyPlaylist.replace_char(',', '.');

		auto find_it = std::find(allowedPlaylists.begin(), allowedPlaylists.end(), dummyPlaylist.c_str());
		if (find_it == allowedPlaylists.end()) {
			// nothing to do
			return false;
		}
	}
	return true;
}
bool bookmark_automatic::upgradeDummy(std::vector<bookmark_t>& masterList, std::list< dlg::CListControlBookmark*> guiLists) {

	if (dummy.desc.length() == 0) {
		// nothing to do
		return false;
	}

	//TODO: what if there is no valid song name?

	metadb_handle_ptr track_bm;
	metadb_handle_ptr track_current;

	//not playing on_quit - is shutting down
	bool bnowPlaying = playback_control_v3::get()->get_now_playing(track_current);
	if (bnowPlaying) {
		track_bm = track_current;
	}
	else {
		auto metadb_ptr = metadb::get();
		track_bm = metadb_ptr->handle_create(dummy.path.c_str(), dummy.subsong);
	}

	auto track_length = track_bm->get_length();
	bool bsamepath = pfc::string8(track_bm->get_path()).equals(dummy.path);
	if (bsamepath) {
		if (dummy.time + 3 >= track_bm->get_length()) {
			// nothing to do
			return false;
		}
	}
	else {
		updateDummy();
	}

	if (!CheckAutoFilter()) {
		FB2K_console_print_v("Filter is active and did not match, do not store a bookmark.");
		return false;
	}

	double timeFuzz = 1.0;

	if (is_cfg_Bookmarking()) {

		for (std::vector<bookmark_t>::iterator it = masterList.begin(); it != masterList.end(); ++it) {
			if ((abs(it->time - dummy.time) <= timeFuzz) &&
				(it->path.equals(dummy.path)) &&
				(it->playlist.equals(dummy.playlist)) &&
				(pfc::guid_equal(it->guid_playlist, dummy.guid_playlist))) {

				// nothing to do
				return false;
			}
		}

		FB2K_console_print_v("AutoBookmark: storing");

		bookmark_t lastmark;
		if (masterList.size()) {
			lastmark = masterList.at(masterList.size() - 1);
		}

		if (lastmark.path.get_length() && lastmark.path.equals(dummy.path)) {
			//if quitting dummy is the last item
			masterList.at(masterList.size() - 1) = dummy;
		}
		else {
			//The filter was either disabled or matched the current playlist, continue:
			bookmark_t copy = bookmark_t(dummy);
			masterList.emplace_back(std::move(copy));	//store the mark
		}

		//Update all gui lists
		if (!core_api::is_shutting_down()) {
			for (auto it = guiLists.begin(); it != guiLists.end(); ++it) {
				dlg::CListControlBookmark* lc = *it;
				lc->OnItemsInserted(masterList.size(), 1, true);
			}
		}
	}
	return true;
}
