#include "stdafx.h"
#include "bookmark_core.h"
#include "bookmark_list_control.h"
#include "bookmark_playlist_callback.h"

using namespace glb;

unsigned bookmark_playlist_callback::get_flags() {
	return flag_playlist_ops;
}

void bookmark_playlist_callback::on_playlists_removing(const bit_array& p_mask, t_size p_old_count, t_size p_new_count) {
	
	if (!cfg_monitor.get()) {
		return;
	}

	if (g_guiLists.empty()) {
		return;
	}

	auto f = p_mask.find_first(true, 0, p_old_count);
	for (auto n = f; n < p_old_count; n = p_mask.find_next(true, n, p_old_count))
	{
		bool bchanged = false;
		pfc::string8 dbgstr;
		playlist_manager_v5::get()->playlist_get_name(n, dbgstr);
		playlist_manager_v5::get()->playlist_get_name(n - 1, dbgstr);

		GUID plguid = playlist_manager_v5::get()->playlist_get_guid(n);
		pfc::string8 str_plguid = pfc::print_guid(plguid);

		bit_array_bittable changeMask(bit_array_false(), g_primaryGuiList->GetItemCount());

		const std::vector<bookmark_t> masterList = g_store.GetMasterList();
		auto& found_it = std::find_if(masterList.begin(), masterList.end(), [&](const bookmark_t& elem) {
			return (pfc::guid_equal(plguid, elem.guid_playlist));
			});

		while (found_it != masterList.end()) {

			bchanged = true;
			/*found_it->guid_playlist = plguid;*/
			auto pos = std::distance(masterList.begin(), found_it);

			changeMask.set(pos, true);

			bookmark_t rec = g_store.GetItem(pos);
			rec.playlist = "";
			rec.guid_playlist = pfc::guid_null;
			g_store.SetItem(pos, rec);

			found_it = std::find_if(found_it + 1, masterList.end(), [&](const bookmark_t& elem) {
				return (pfc::guid_equal(plguid, elem.guid_playlist));
				});
		}

		if (bchanged) {

			for (auto gui : g_guiLists) {

				//mask positions from master list, not get_selected()

				if (gui->GetSortOrder()) {
					gui->GetSortOrderedMask(changeMask);
				}
				gui->ReloadItems(changeMask);
			}
			g_store.Write();
		}
	}
}

void bookmark_playlist_callback::on_playlist_renamed(t_size p_index, const char* p_new_name, t_size p_new_name_len) {

	if (!cfg_monitor.get()) {
		return;
	}

	if (g_guiLists.empty()) {
		return;
	}

	bool bchanged = false;

	GUID plguid = playlist_manager_v5::get()->playlist_get_guid(p_index);
	pfc::string8 str_plguid = pfc::print_guid(plguid);
	bit_array_bittable changeMask(bit_array_false(), g_primaryGuiList->GetItemCount());

	const std::vector<bookmark_t> masterList = g_store.GetMasterList();
	auto & found_it = std::find_if(masterList.begin(), masterList.end(), [&](const bookmark_t& elem) {
		return (pfc::guid_equal(plguid, elem.guid_playlist));
		});

	while (found_it != masterList.end()) {
		
		bchanged = true;
		
		auto pos = std::distance(masterList.begin(), found_it);
		
		bookmark_t rec = g_store.GetItem(pos);
		rec.playlist = pfc::string8(p_new_name).c_str();
		g_store.SetItem(pos, rec);

		changeMask.set(pos, true);

		found_it = std::find_if(found_it+1, masterList.end(), [&](const bookmark_t& elem) {
			return (pfc::guid_equal(plguid, elem.guid_playlist));
			});
	}
	if (bchanged) {
		for (auto gui : g_guiLists) {
			
			//mask positions from master list, not get_selected()
			if (gui->GetSortOrder()) {
				gui->GetSortOrderedMask(changeMask);
			}

			gui->ReloadItems(changeMask);
		}
		g_store.Write();
	}
}


static service_factory_single_t< bookmark_playlist_callback > bookmark_playlist_callback_service;