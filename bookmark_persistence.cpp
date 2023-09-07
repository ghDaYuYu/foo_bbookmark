#include "stdafx.h"

#include <filesystem>

#include <string>
#include <sstream>

#include "jansson.h"

#include "bookmark_persistence.h"
#include "bookmark_preferences.h"

bookmark_persistence::bookmark_persistence() {
	//..
}

bookmark_persistence::~bookmark_persistence()
{
	//..
}

//replaces the contents of masterList with the contents of the persistent file

void bookmark_persistence::replaceMasterList(std::vector<bookmark_t>& newContent, std::vector<bookmark_t>& masterList) {
	FB2K_console_print_v("Replacing cache");

	masterList.clear();
	masterList.insert(masterList.begin(), newContent.begin(), newContent.end());
}

pfc::string8 bookmark_persistence::genFilePath() {

	static std::string path;

	if (path.empty()) {
		path = std::string(core_api::get_profile_path()).append("\\configuration\\").append(core_api::get_my_file_name()).append(".dll.dat").substr(7, std::string::npos);
	}

	return pfc::string8(path.c_str());
}

void add_rec(std::vector<json_t*> &vjson, const std::vector<pfc::string8>& vlbl, const bookmark_t & rec) {

	const int nfields = static_cast<int>(vlbl.size());

	//bookmark_t to vrec

	std::vector<pfc::string8> vrec = {
		std::to_string(rec.time).c_str(),              //time
		rec.desc.get_ptr(),                            //desc
		rec.playlist.get_ptr(),                        //playlist
		pfc::print_guid(rec.guid_playlist).get_ptr(),  //guid
		rec.path.get_ptr(),                            //path
		std::to_string(rec.subsong).c_str(),           //subsong
		rec.comment.get_ptr(),                         //comment
		rec.date.get_ptr()                             //date
	};

	vjson.emplace_back(json_object());

	auto nobj = vjson.size() - 1;
	for (auto wfield = 0; wfield < nfields; wfield++) {
		//recycle pointers
		int res = json_object_set_new_nocheck(vjson[nobj], vlbl[wfield], json_string_nocheck(vrec[wfield].get_ptr()));
	}
}

//save masterList to persistent storage
void bookmark_persistence::writeDataFileJSON(std::vector<bookmark_t>& masterList) {

	if (core_api::is_quiet_mode_enabled()) {
		FB2K_console_print_e("Quiet mode, will not write bookmarks to file");
		return;
	}

	if (!masterList.size()) {
		std::filesystem::path os_file_name = std::filesystem::u8path(genFilePath().c_str());
		if (std::filesystem::exists(os_file_name)) {
			std::error_code ec;
			std::filesystem::remove(os_file_name, ec);
		}
		return;
	}

	try {

		FB2K_console_print_v("Preparing to write data file.");

		size_t n_entries = masterList.size();

		std::vector<json_t*> vjson;
		std::vector<pfc::string8> vlbl = { "time", "desc", "playlist", "guid", "path", "subsong", "comment", "date" };

		//first pass

		for (size_t i = 0; i < n_entries; i++) {
			//todo: recycle pointer
			//add boolmark_t to vjson as json object
			add_rec(vjson, vlbl, masterList[i]);
		}

		//second pass, could have be done yet in first pass

		json_t* arr_top = json_array();

		for (auto wobj : vjson) {

			auto res = json_array_append(arr_top, wobj);
		}

		// dump object array

		std::filesystem::path os_root = genFilePath().c_str();

		auto res = json_dump_file(arr_top, os_root.u8string().c_str(), JSON_INDENT(5));

		for (auto w : vjson) {
			free(w);
		}

		FB2K_console_print_v("Wrote ", std::to_string(n_entries).c_str(), " bookmarks to file");
	}
	catch (foobar2000_io::exception_io e) {
		FB2K_console_print_e("Could not write bookmarks to file", e);
	}
	catch (...) {
		FB2K_console_print_e("Could not write bookmarks to file", "Unhandled Exception");
	}
}

//restore masterList from persistent storage
bool bookmark_persistence::readDataFileJSON(std::vector<bookmark_t>& masterList) {

	try {

		FB2K_console_print_v("Reading bookmarks from file");

		size_t clines = 0;
		std::vector<bookmark_t> temp_data;

		try {
		
			json_error_t error;
			auto json = json_load_file(genFilePath().c_str(), JSON_DECODE_ANY, &error);

			size_t clines = json_array_size(json);

			bookmark_t elem = bookmark_t();

			size_t index;
			json_t* js_wobj;

			json_array_foreach(json, index, js_wobj) {

				if (!json_is_object(js_wobj)) {
					break;
				}

				json_t* js_fld;
				{
					elem.time = 0;
					json_t* js_fld = json_object_get(js_wobj, "time");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						elem.time = atof(dmp_str);
					}
				}

				{
					elem.guid_playlist = pfc::guid_null;
					js_fld = json_object_get(js_wobj, "guid");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						pfc::string8 tmpguid = pfc::string8(dmp_str);
						elem.guid_playlist = pfc::GUID_from_text(tmpguid);
					}
				}

				{
					elem.desc = "";
					js_fld = json_object_get(js_wobj, "desc");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						elem.desc = pfc::string8(dmp_str);
					}
				}

				{
					elem.path = "";
					js_fld = json_object_get(js_wobj, "path");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						elem.path = pfc::string8(dmp_str);
					}

				}

				{
					elem.subsong = 0;
					js_fld = json_object_get(js_wobj, "subsong");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						elem.subsong = static_cast<t_uint32>(json_real_value(js_fld));
					}
				}

				{
					elem.comment = "";
					js_fld = json_object_get(js_wobj, "comment");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						elem.comment = pfc::string8(dmp_str);
					}

				}

				{
					elem.date = "";
					js_fld = json_object_get(js_wobj, "date");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						elem.date = pfc::string8(dmp_str);
					}

				}

				{
					elem.playlist = "";
					js_fld = json_object_get(js_wobj, "playlist");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						elem.playlist = pfc::string8(dmp_str);
					}
				}

				// check playlist name
				pfc::string8 playlist_name_from_guid;
				if (core_version_info_v2::get()->test_version(2, 0, 0, 0)) {
					size_t ndx = playlist_manager_v5::get()->find_playlist_by_guid(elem.guid_playlist);
					if (ndx != pfc_infinite) {
						playlist_manager_v5::get()->playlist_get_name(ndx, playlist_name_from_guid);
					}
				}

				if (playlist_name_from_guid.get_length()) {
					//replace old playlist name
					if (!elem.playlist.equals(playlist_name_from_guid)) {
						elem.playlist = playlist_name_from_guid;
					}
				}

				temp_data.push_back(elem);	//to vector

			}

			if (cfg_verbose) {
				FB2K_console_print_v("File content:");
				for (size_t i = 0; i < temp_data.size(); ++i)
					FB2K_console_print_v("time ", i, ": ", temp_data[i].time);
			}

		}
		catch (foobar2000_io::exception_io e) {
			FB2K_console_print_e("Reading data from file failed", e);
			return false;
		}
		catch (...) {
			FB2K_console_print_e("Reading data from file failed", "Unhandled Exception");
			return false;
		}

		FB2K_console_print_v("Restored ", std::to_string(clines).c_str(), " bookmarks from file");

		//update masteList
		replaceMasterList(temp_data, masterList);
	}
	catch (...) {
		
		//..
	}
	return true;
}

