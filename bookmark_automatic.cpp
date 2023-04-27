#include "stdafx.h"
#include "bookmark_automatic.h"

bookmark_automatic::bookmark_automatic() {

}

bookmark_automatic::~bookmark_automatic() {}

//Only update the time stored in the dummy - assumes that we will be notified of song changes
void bookmark_automatic::updateDummyTime() {
	dummy.m_time = playback_control::get()->playback_get_position();

	if (m_updatePlaylist) {
		m_updatePlaylist = false;

		pfc::string_fixed_t<80> playing_pl_name;
		size_t index_playlist;
		size_t index_item;
		auto playlist_manager_ptr = playlist_manager::get();
		if (cfg_bookmark_verbose) {
			if (playlist_manager_ptr->get_playing_item_location(&index_playlist, &index_item)) {
				FB2K_console_print("AutoBookmark: dummy update: Playlist index is ", index_playlist);
				if (playlist_manager_ptr->playlist_get_name(index_playlist, playing_pl_name))
					FB2K_console_print("AutoBookmark: dummy update: Playlist name is ", playing_pl_name);
			} else {
				FB2K_console_print("AutoBookmark: dummy update: couldn't find playlist index");
			}
		}

		//Set the flag back to true if either operation fails
		m_updatePlaylist |= !playlist_manager_ptr->get_playing_item_location(&index_playlist, &index_item);
		m_updatePlaylist |= !playlist_manager_ptr->playlist_get_name(index_playlist, playing_pl_name);

		dummy.m_playlist = playing_pl_name.c_str();	//without using c_str(), the full 80 characters are written every time
	}
}

//Fully update the dummy.
void bookmark_automatic::updateDummy() {
	pfc::string_formatter songDesc;

	metadb_handle_ptr dbHandle_item;
	auto playback_control_ptr = playback_control::get();
	playback_control_ptr->get_now_playing(dbHandle_item);

	titleformat_object::ptr desc_format;
	static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(desc_format, cfg_bookmark_desc_format.c_str());

	if (!dbHandle_item->format_title(NULL, songDesc, desc_format, NULL)) {
		songDesc << "Could not generate Description.";
	}

	m_updatePlaylist = true;	//We can't read the PlName right after the track was changed

	pfc::string songPath = dbHandle_item->get_path();

	//TODO: graceful failure?!

	dummy.m_time = playback_control_ptr->playback_get_position();
	dummy.m_desc = songDesc;
	dummy.m_path = songPath;
	dummy.m_subsong = dbHandle_item->get_subsong_index();
}

bool bookmark_automatic::upgradeDummy(std::vector<bookmark_t>& masterList, std::list<CListControlBookmark *> & guiLists) {
	if (dummy.m_desc.length() == 0)
		return false;
	//TODO: what if there is no valid song name?

	//Fizzle out if we have reached the end of the track
	auto metadb_ptr = metadb::get();
	metadb_handle_ptr track_bm = metadb_ptr->handle_create(dummy.m_path.c_str(), dummy.m_subsong);
	if (dummy.m_time + 3 >= track_bm->get_length()) {
		return false;
	}


	if (cfg_bookmark_autosave_newTrackFilter) {
		//Filter is active, check for a match:

		if (cfg_bookmark_verbose) FB2K_console_print("AutoBookmark: The dummie's playlist is called ", dummy.m_playlist.c_str());

		//Obtain individual names in the filter
		std::vector<std::string> allowedPlaylists;
		std::stringstream ss(cfg_bookmark_autosave_newTrack_playlists.c_str());
		std::string token;
		while (std::getline(ss, token, ',')) {
			allowedPlaylists.push_back(token);
		}

		//replace all , in the name of the current playlist with .
		pfc::string8 dummyPlaylist = dummy.m_playlist.c_str();
		dummyPlaylist.replace_char(',', '.');


		//compare the contents of the filter
		bool matchFound = false;
		for (std::string ap : allowedPlaylists) {
			if (cfg_bookmark_verbose) FB2K_console_print("...comparing with filter: ", ap.c_str());
			if (strcmp(ap.c_str(), dummyPlaylist.c_str()) == 0) {
				matchFound = true;
				if (cfg_bookmark_verbose) FB2K_console_print("...matches.");
				break;
			}
		}
		if (!matchFound) {
			if (cfg_bookmark_verbose) FB2K_console_print("...no match.");
			return false;	//Filter is active and did not match, do not store a bookmark
		}
	}

	double timeFuzz = 1.0;
	for (std::vector<bookmark_t>::iterator it = masterList.begin(); it != masterList.end(); ++it) {
		if ((abs(it->m_time - dummy.m_time) <= timeFuzz) &&
			(it->m_path == dummy.m_path) &&
			(it->m_playlist == dummy.m_playlist)) {
			return false;
		}
	}

	FB2K_console_print("AutoBookmark: storing");
	//The filter was either disabled or matched the current playlist, continue:
	bookmark_t copy = bookmark_t(dummy);
	masterList.emplace_back(copy);	//store the mark

	//Update each gui List
	for (std::list<CListControlBookmark *>::iterator it = guiLists.begin(); it != guiLists.end(); ++it) {
		(*it)->OnItemsInserted(masterList.size(), 1, true);
	}
	return true;
}