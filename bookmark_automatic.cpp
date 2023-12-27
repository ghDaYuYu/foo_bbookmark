#include "stdafx.h"

#include "SDK/playback_control.h"
#include "SDK/playlist.h"

//update playlist
#include "bookmark_core.h"
#include "bookmark_store.h"
#include "bookmark_automatic.h"
#include "bookmark_list_control.h"
#include "bookmark_preferences.h"

using namespace glb;

void bookmark_automatic::updateDummyTime() {

	auto oldtime = dummy.get_time();
	dummy.set_rt_time(playback_control::get()->playback_get_position());

	if (m_updatePlaylist) {
		m_updatePlaylist = false;

		auto playlist_manager_ptr = playlist_manager_v5::get();
		pfc::string_fixed_t<80> playing_pl_name;
		size_t index_playlist;
		size_t index_item;

		bool bItemLoc = playlist_manager_ptr->get_playing_item_location(&index_playlist, &index_item);
		bool bPlaylist = false;

		if (bItemLoc) {
			FB2K_console_print_v("<update time> - Item location playlist index: ", index_playlist);

			bPlaylist = playlist_manager_ptr->playlist_get_name(index_playlist, playing_pl_name);

			if (bPlaylist) {
				FB2K_console_print_v("<update time> - Item location playlist name: ", playing_pl_name);
			}
		}
		else {
			FB2K_console_print_v("<update time> - Fetching item location ", dummy.path);

			//increase retries
			++dummy.need_loc_retries;
		}

		//Set the flag back to true if either operation fails
		m_updatePlaylist |= !bItemLoc;
		m_updatePlaylist |= !bPlaylist;

		bool bcan_autosave_newtrack = cfg_autosave_newtrack.get() && (!checkDummyIsRadio() || cfg_autosave_radio_newtrack.get());

		if (!m_updatePlaylist) {

			//without using c_str(), the full 80 characters are written every time
			dummy.playlist = playing_pl_name.c_str();
			dummy.guid_playlist = playlist_manager_ptr->playlist_get_guid(index_playlist);
			dummy.need_playlist = m_updatePlaylist;

			if (!dummy.need_playlist) {
				FB2K_console_print_v("Track details ready... ", dummy.desc);
			}
			else {
				return;
			}

			if (bcan_autosave_newtrack) {
				// AUTO - CREATE
				bool bres = upgradeDummy(g_guiLists);
				bres = bres;
			}
		}
		else {

			if (dummy.need_loc_retries > LOC_RETRIES) {
				FB2K_console_print_v("<update time>: giving up item location requests");

				//give up
				m_updatePlaylist = dummy.need_playlist = false;

				if (!dummy.need_playlist) {
					FB2K_console_print_v("Track partial details ready... ", dummy.desc);
				}
				else {
					return;
				}

				if (bcan_autosave_newtrack) {
					// AUTO - CREATE
					bool bres = upgradeDummy(g_guiLists);
				}
				return;
			}
		}
	}
}

bool bookmark_automatic::fetchHelloRadioStationName(pfc::string8& out) {
	if (!checkDummyIsRadio())
	{
		return false;
	}

	if (cfg_autosave_radio_comment.get()) {
		pfc::string_formatter sfTitle;
		if (m_pttf_title.is_empty()) {
			static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_pttf_title, "%title%");
		}
		if (playback_control::get()->playback_format_title(NULL, sfTitle, m_pttf_title, NULL, playback_control::display_level_titles)) {
			if (sfTitle.get_length() > 1) {
				out = sfTitle;
				return true;
			}
		}
	}
	return false;
}

//Full update
void bookmark_automatic::updateDummy() {

	metadb_handle_ptr dbHandle_item;
	auto playback_control_ptr = playback_control::get();

	if (playback_control_ptr->get_now_playing(dbHandle_item)) {

		bool blocation_ok = false;

		pfc::string_formatter songDesc;
		titleformat_object::ptr desc_format;
		static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(desc_format, cfg_desc_format.get_value().c_str());

		pfc::string8 songPath = dbHandle_item->get_path();
		if (checkDummyIsRadio(songPath)) {
			blocation_ok = playback_control::get()->playback_format_title(NULL, songDesc, desc_format, NULL, playback_control::display_level_all);
			if (!blocation_ok) {
				songDesc << "Could not generate Description.";
			}
		}
		else {
			blocation_ok = dbHandle_item->format_title(NULL, songDesc, desc_format, NULL);
			if (!blocation_ok) {
				songDesc << "Could not generate Description.";
			}
		}

		size_t index_playlist;
		pfc::string8 playing_playlist_name;
		GUID guid_playing_playlist = pfc::guid_null;

		auto pl_man = playlist_manager_v5::get();

		size_t index_playing_playlist = pl_man->get_playing_playlist();
		bool has_playlist = index_playing_playlist != SIZE_MAX;

		size_t index_item;
		has_playlist &= pl_man->playlist_find_item(index_playing_playlist, dbHandle_item, index_item);

		if (has_playlist) {

			blocation_ok &= pl_man->get_playing_item_location(&index_playlist, &index_item);
			blocation_ok &= pl_man->playlist_get_name(index_playlist, playing_playlist_name);

			if (blocation_ok) {
				FB2K_console_print_v("<update dummy> - Item location: ", playing_playlist_name);
				guid_playing_playlist = pl_man->playlist_get_guid(index_playlist);
			}
			else {
				FB2K_console_print_v("<update dummy> - Fetching item location...");
			}
		}
		else {

			//todo: radio station without playlist ???
			if (dummy.isRadio(songPath) && !dummy.desc.get_length() && !dummy.dyna) {
				blocation_ok = false;
			}
			else {
				blocation_ok = true;
			}
		}

		m_updatePlaylist = !blocation_ok;
		m_updatePlaylist &= dummy.need_loc_retries <= LOC_RETRIES;

		//TODO: graceful failure?!

		dummy.set_time(playback_control_ptr->playback_get_position());
		dummy.path = songPath;

		if (!dummy.isRadio()) {
			dummy.desc = songDesc;
		}
		else {
			
				if (!dummy.desc.get_length()) {
					dummy.desc = songDesc;
				}
				else {
					if (dummy.dyna) {
						dummy.desc = songDesc;
					}
				}
		}

		dummy.subsong = dbHandle_item->get_subsong_index();
		dummy.playlist = playing_playlist_name;
		dummy.guid_playlist = guid_playing_playlist;
		gimme_date(dummy);

		//dyna
		pfc::string8 station_name;
		if (!dummy.comment.get_length() && fetchHelloRadioStationName(station_name)) {
			dummy.comment = station_name;
		}

		if (m_updatePlaylist) {
			++dummy.need_loc_retries;
		}
		dummy.need_playlist = m_updatePlaylist && dummy.need_loc_retries <= LOC_RETRIES;

	}
	else {
		if (!core_api::is_shutting_down()) {
			dummy.reset();
		}
	}
}

bool bookmark_automatic::CheckAutoFilter() {
	if (cfg_autosave_filter_newtrack.get()) {
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
bool bookmark_automatic::upgradeDummy(std::list< dlg::CListControlBookmark*> guiLists) {

	const std::vector<bookmark_t>& masterList = g_store.GetMasterList();

	if (dummy.need_playlist) {
		return false;
	}

	size_t old_size = masterList.size();

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

	auto track_subsong = bnowPlaying ? track_current->get_subsong_index() : track_bm->get_subsong_index();
	auto track_length = track_bm->get_length();
	bool bsamepath = pfc::string8(track_bm->get_path()).equals(dummy.path);
	bsamepath &= track_subsong == dummy.subsong;

	bool bsame_radio_content = true;

	pfc::string_formatter songDesc;
	bool myPendingRadioDyna = false;
	
	//todo: unify with local files which are rejected by duplicated criteria
	bool bradio_restored = isRestoredRadioDummy(dummy);
	bool bdummy_restored = isRestoredDummy(dummy);

	if (!core_api::is_shutting_down()) {
		if (bsamepath && (bradio_restored || bdummy_restored)) {
				return false;
			}
		}
		else {
			//updateDummy();
		}
	}

	if (!CheckAutoFilter()) {
		if (cfg_verbose) console::formatter() << "Filter is active and did not match, do not store a bookmark.";
		return false;
	}


	if (is_cfg_Bookmarking()) {

		for (auto rit = std::rbegin(masterList); rit != std::rend(masterList); ++rit) {

			bool brevtime = abs(rit->get_time() - dummy.get_time()) <= 2 * KMin_Lapse;
			bool brevpath = rit->path.equals(dummy.path) && pfc::guid_equal(rit->guid_playlist, dummy.guid_playlist);
			brevpath = brevpath && rit->subsong == dummy.subsong;
			bool brevradio = !dummy.isRadio() || (rit->desc.equals(dummy.desc));

			if (dummy.need_playlist || (brevtime && brevpath && brevradio)) {

				if (cfg_verbose) {
					if (!dummy.need_playlist && (brevtime && brevpath)) {
						FB2K_console_print_v("Skipping duplicated bookmark: ", dummy.path);
					}
				}

				// nothing to do
				return false;
			}
		}

		bool bshooting_down = core_api::is_shutting_down();
		if (bshooting_down && masterList.size()) {
			//..
		}
		else {
			//The filter was either disabled or matched the current playlist, continue:
			g_store.AddItem(std::move(bookmark_t(dummy)));
			FB2K_console_print_v("Dummy stored");
		}

		//UI update
		if (!bshooting_down) {
			bool bscroll_list = cfg_autosave_focus_newtrack.get();
			refresh_ui(bscroll_list, bscroll_list, g_store.GetMasterList(), guiLists);
		}
	}
	return g_store.Size() != old_size;
}

void bookmark_automatic::ResetRestoredDummy() {
	restored_dummy.reset();
}

void bookmark_automatic::ResetRestoredDummyTime() {
	restored_dummy.set_time(0.0);
}

void bookmark_automatic::SetRestoredDummy(bookmark_t& bm) {
	restored_dummy = bookmark_t(bm);
}

bool bookmark_automatic::isRestoredDummy(const bookmark_t& bm) {

	if (restored_dummy.get_time() || pfc::guid_equal(restored_dummy.guid_playlist, bm.guid_playlist) &&
		(restored_dummy.path.equals(bm.path)) && abs(restored_dummy.get_time() - bm.get_time()) <= 3) {
		return true;
	}
	return false;
}

bool bookmark_automatic::isRestoredRadioDummy(const bookmark_t& bm) {

	if (bm.isRadio() && pfc::guid_equal(restored_dummy.guid_playlist, bm.guid_playlist) &&
		(restored_dummy.path.equals(bm.path)) && restored_dummy.desc.equals(bm.desc)) {
		return true;
	}
	return false;
}

void bookmark_automatic::checkDeletedRestoredDummy(const bit_array& mask, size_t count) {

	auto f = mask.find_first(true, 0, count);
	if (f >= count) {
		ResetRestoredDummy();
	}
	else {
		for (auto n = f; n < count; n = mask.find_next(true, n, count)) {
			bookmark_t tmpbm = g_store.GetItem(n);
			if (isRestoredDummy(tmpbm)) {
				ResetRestoredDummy();
				break;
			}
		}
	}
}

void bookmark_automatic::refresh_ui(bool bselect, bool bensure_visible, const std::vector<bookmark_t>& masterList, std::list< dlg::CListControlBookmark*> guiLists) {
	for (auto it = guiLists.begin(); it != guiLists.end(); ++it) {
		dlg::CListControlBookmark* lc = *it;
		size_t item = masterList.size() - 1;
		if (lc->GetSortOrder()) {
			item = 0;
		}

		if (bselect) {
			lc->SelectNone();
		}
		lc->OnItemsInserted(item, 1, bselect);
		if (bselect || bensure_visible) {
			lc->EnsureItemVisible(item, false);
		}
		if (bselect) {
			lc->SetFocusItem(item);
		}
	}
}
