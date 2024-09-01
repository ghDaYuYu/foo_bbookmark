#include "stdafx.h"

#include <string>
#include <fcntl.h>
#include <share.h>
#include <io.h>

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

std::filesystem::path bookmark_persistence::genFilePath() {

	pfc::string8 n8_path = core_api::pathInProfile("configuration");
	extract_native_path(n8_path, n8_path);

	std::filesystem::path os_dst = std::filesystem::u8path(n8_path.c_str());

	pfc::string8 filename = core_api::get_my_file_name();
	filename << ".dll.dat";

	os_dst.append(filename.c_str());

	return os_dst;
}

void add_rec(std::vector<json_t*> &vjson, const std::vector<pfc::string8>& vlbl, const bookmark_t & rec) {

	const int nfields = static_cast<int>(vlbl.size());

	//bookmark_t to vrec

	std::vector<pfc::string8> vrec = {

		pfc::print_guid(rec.guid_bm).get_ptr(),        //guid_bm
		std::to_string(rec.get_time()).c_str(),        //time
		rec.desc.get_ptr(),                            //desc
		rec.get_name(false).get_ptr(),                 //name
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
		if (vlbl[wfield].equals("name")) {
			if (!vrec[wfield].get_length()) {
				continue;
			}
		}
		int res = json_object_set_new_nocheck(vjson[nobj], vlbl[wfield], /*vjs_val[i]*/json_string_nocheck(vrec[wfield].get_ptr()));
	}
}

void bookmark_persistence::writeDataFile(const std::vector<bookmark_t>& masterList) {
	cmdThFile.add([this, &masterList] { writeDataFileJSON(masterList); });
}

//save masterList to persistent storage
void bookmark_persistence::writeDataFileJSON(const std::vector<bookmark_t>& masterList) {

	if (core_api::is_quiet_mode_enabled()) {
		FB2K_console_print_e("Quiet mode, will not write bookmarks to file");
		return;
	}
		if (!masterList.size()) {
			std::filesystem::path os_file_name = genFilePath();
			if (std::filesystem::exists(os_file_name)) {
				std::error_code ec;
				std::filesystem::remove(os_file_name, ec);
			}
			return;
		}


		int jf = -1;

		try {

			FB2K_console_print_v("Write data file...");

			size_t n_entries = masterList.size();

			std::vector<json_t*> vjson;
			std::vector<pfc::string8> vlbl = { "guid_bm", "time", "desc", "name", "playlist", "guid", "path", "subsong", "comment", "date" };

			//first pass

			for (size_t i = 0; i < n_entries; i++) {
				//add boolmark_t to vjson as json object
				add_rec(vjson, vlbl, masterList[i]);
			}

			//second pass, could have be done in first pass

			json_t* arr_top = json_array();

			for (auto wobj : vjson) {
				auto res = json_array_append(arr_top, wobj);
			}

			std::filesystem::path os_file = genFilePath();
			jf = _wopen(os_file.wstring().c_str(), _O_CREAT | _O_TRUNC | _O_RDWR | _O_TEXT/*_O_U8TEXT*/, _S_IWRITE);

			if (jf == -1) {
				foobar2000_io::exception_io e("Open failed on output file");
				throw e;
			}

			auto res = json_dumpfd(arr_top, jf, JSON_INDENT(5));
			_close(jf);

			for (auto w : vjson) {
				free(w);
			}

			FB2K_console_print_v("Wrote ", std::to_string(n_entries).c_str(), " bookmarks to file");
		}
		catch (foobar2000_io::exception_io e) {
			if (jf != -1) {
				_close(jf);
			}
			FB2K_console_print_e("Could not write bookmarks to file", e);
		}
		catch (...) {
			if (jf != -1) {
				_close(jf);
			}
			FB2K_console_print_e("Could not write bookmarks to file", "Unhandled Exception");
		}
	//}
}

//restore masterList from persistent storage
bool bookmark_persistence::readDataFileJSON(std::vector<bookmark_t>& masterList) {
	try {

		FB2K_console_print_v("Reading bookmarks from file");

		size_t clines = 0;
		std::vector<bookmark_t> temp_data;

		int jf = -1;

		try {
		
			std::filesystem::path os_file = genFilePath();
			jf = _wopen(os_file.wstring().c_str(), _O_RDONLY);

			if (jf == -1) {
				foobar2000_io::exception_io e("Open failed on input file");
				throw e;
			}

			json_error_t error;
			auto json = json_loadfd(jf, JSON_DECODE_ANY, &error);
			_close(jf);

			auto json = json_load_file(os_file.generic_string().c_str(), JSON_DECODE_ANY, &error);
			if (strlen(error.text) && error.line != -1) {
				FB2K_console_print_v("JSON error: ",error.text,
						" in line: ", error.line,
						", column: ", error.column,
						", position: ", error.position,
						", src: ",error.source);
				try {
					if (std::filesystem::file_size(os_file.c_str())) {
						FB2K_console_print_v("JSON error: Creating backup file...");
						pfc::string8 buffer_bak;
						buffer_bak << os_file.c_str() << ".bak";
						std::filesystem::copy(os_file, buffer_bak.c_str(), std::filesystem::copy_options::update_existing);
						FB2K_console_print_v("JSON error: Backup file created.");
					}
				}
				catch (std::filesystem::filesystem_error const& ex) {
					FB2K_console_print_v("Error: Creating backup file. ", ex.what());
				}
				//free(error);
			}
			clines = json_array_size(json);

			bookmark_t elem = bookmark_t();

			size_t index;
			json_t* js_wobj;

			json_array_foreach(json, index, js_wobj) {

				if (!json_is_object(js_wobj)) break;

				json_t* js_fld;

				{
					elem.guid_playlist = pfc::guid_null;
					js_fld = json_object_get(js_wobj, "bm_guid");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						pfc::string8 tmpguid = pfc::string8(dmp_str);
						elem.guid_bm = pfc::GUID_from_text(tmpguid);
					}
					else {
						elem.guid_bm = pfc::createGUID();
					}
				}

				{
					elem.set_time(0);
					json_t* js_fld = json_object_get(js_wobj, "time");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						elem.set_time(atof(dmp_str));
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
					elem.set_name("");
					js_fld = json_object_get(js_wobj, "name");
					const char* dmp_str = json_string_value(js_fld);
					if (dmp_str) {
						elem.set_name(dmp_str);
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
						elem.subsong = static_cast<t_uint32>(atoi(dmp_str));
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

					if (dmp_str && strlen(dmp_str) > 20) {
						// Www Mmm dd hh:mm:ss yyyy
						time_t rawtime;
						struct tm timeinfo = { 0 };

						unix_str_date_to_time(dmp_str, rawtime, timeinfo);
						elem.date = pfc::string8(dmp_str);
						char buffer[DATE_BUFFER_SIZE];
						// Format: Mo, 15.06.2009 20:20:00
						std::tm ptm;
						localtime_s(&ptm, &rawtime);
						std::strftime(buffer, DATE_BUFFER_SIZE, cfg_date_format.get_value(), &ptm);
						elem.runtime_date = buffer;
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

				size_t ndx = playlist_manager_v5::get()->find_playlist_by_guid(elem.guid_playlist);
				if (ndx != pfc_infinite) {
					playlist_manager_v5::get()->playlist_get_name(ndx, playlist_name_from_guid);
				}

				if (playlist_name_from_guid.get_length()) {
					//replace old playlist name
					if (!elem.playlist.equals(playlist_name_from_guid)) {
						elem.playlist = playlist_name_from_guid;
					}
				}

				//recover non-cue tracks from swap subsong bug (v1.3.1)
				std::filesystem::path p(elem.path.c_str());
				auto ext = p.extension().string();
				if (p.extension().string().compare(".cue")) {
					//not cue
					if (elem.subsong) {
						elem.subsong = 0;
					}
				}
				else {
					//cue
					if (!elem.subsong) {
						//not recoverable, default to 1
						elem.subsong = 1;
					}
				}

				temp_data.push_back(elem);	//save to vector
			}
		}
		catch (foobar2000_io::exception_io e) {
			if (jf != -1) {
				_close(jf);
			}
			FB2K_console_print_e("Reading data from file failed", e);
			return false;
		}
		catch (...) {
			if (jf != -1) {
				_close(jf);
			}
			FB2K_console_print_e("Reading data from file failed:", " Unhandled Exception");
			return false;
		}

		FB2K_console_print_v("Restored ", std::to_string(clines).c_str(), " bookmarks from file");

		replaceMasterList(temp_data, masterList);
	}
	catch (...) {
		//..
	}

	return true;
}

