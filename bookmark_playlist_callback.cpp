#include "stdafx.h"

#include "bookmark_core.h"

#include "bookmark_list_control.h"
#include "bookmark_playlist_callback.h"

unsigned bookmark_playlist_callback::get_flags() {
	return flag_playlist_ops;
}

void bookmark_playlist_callback::on_playlists_removing(const bit_array& p_mask, t_size p_old_count, t_size p_new_count) {
	//todo
}

void bookmark_playlist_callback::on_playlists_removed(const bit_array& p_mask, t_size p_old_count, t_size p_new_count) {
	//todo
}

void bookmark_playlist_callback::on_playlist_renamed(t_size p_index, const char* p_new_name, t_size p_new_name_len) {

	if (g_guiLists.empty()) {
		return;
	}

	bool bchanged = false;

	GUID plguid = playlist_manager_v5::get()->playlist_get_guid(p_index);
	pfc::string8 str_plguid = pfc::print_guid(plguid);
	bit_array_bittable changeMask(bit_array_false(), g_primaryGuiList->GetItemCount());

	auto & found_it = std::find_if(g_masterList.begin(), g_masterList.end(), [&](const bookmark_t& elem) {
		return (pfc::guid_equal(plguid, elem.guid_playlist));
		});

	while (found_it != g_masterList.end()) {
		bchanged = true;
		found_it->playlist = pfc::string8(p_new_name).c_str();
		auto pos = std::distance(g_masterList.begin(), found_it);
		changeMask.set(pos, true);

		found_it = std::find_if(found_it+1, g_masterList.end(), [&](const bookmark_t& elem) {
			return (pfc::guid_equal(plguid, elem.guid_playlist));
			});
	}
	if (bchanged) {
		for (auto gui : g_guiLists) {
			gui->ReloadItems(changeMask);
		}
	}


	//ffTRACK_CALL_TEXT_DEBUG("bookmark_playlist_callback::on_playlists_removed");
	//static_api_ptr_t<playlist_history>()->on_playlist_renamed(p_index, p_new_name, p_new_name_len);
}


static service_factory_single_t< bookmark_playlist_callback > bookmark_playlist_callback_service;