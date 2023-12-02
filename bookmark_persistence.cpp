#include "stdafx.h"

#include <filesystem>
#include <string>
#include <sstream>
#include <map>

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

	static pfc::string8 path;

	if (path.empty()) {
		path = core_api::get_profile_path();
		path << "\\configuration\\";
		path << core_api::get_my_file_name();
		path << ".dll.dat";
		foobar2000_io::extract_native_path(path, path);
	}

	return path;
}

void add_rec(std::vector<json_t*> &vjson, const std::vector<pfc::string8>& vlbl, const bookmark_t & rec) {

	const int nfields = static_cast<int>(vlbl.size());

	//bookmark_t to vrec

	std::vector<pfc::string8> vrec = {
		std::to_string(rec.get_time()).c_str(),        //time
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
		int res = json_object_set_new_nocheck(vjson[nobj], vlbl[wfield], json_string_nocheck(vrec[wfield].get_ptr()));
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
			std::filesystem::path os_file_name = std::filesystem::u8path(genFilePath().c_str());
			if (std::filesystem::exists(os_file_name)) {
				std::error_code ec;
				std::filesystem::remove(os_file_name, ec);
			}
			return;
		}

		try {

			FB2K_console_print_v("Write data file...");

			size_t n_entries = masterList.size();

			std::vector<json_t*> vjson;
			std::vector<pfc::string8> vlbl = { "time", "desc", "playlist", "guid", "path", "subsong", "comment", "date" };

			//first pass

			for (size_t i = 0; i < n_entries; i++) {
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
	//}
}

int get_month_index(std::string name)
{
	std::map<std::string, int> months
	{
			{ "Jan", 1 },
			{ "Feb", 2 },
			{ "Mar", 3 },
			{ "Apr", 4 },
			{ "May", 5 },
			{ "Jun", 6 },
			{ "Jul", 7 },
			{ "Aug", 8 },
			{ "Sep", 9 },
			{ "Oct", 10 },
			{ "Nov", 11 },
			{ "Dec", 12 }
	};

	const auto iter = months.find(name);

	if (iter != months.cend())
		return iter->second;
	return 1/*SIZE_MAX*/;
}

int get_wday_index(std::string name)
{
	std::map<std::string, int> wdays
	{
			{ "Mon", 0 },
			{ "Tue", 1 },
			{ "Wed", 2 },
			{ "The", 3 },
			{ "Fri", 4 },
			{ "Sat", 5 },
			{ "Sun", 6 },
	};

	const auto iter = wdays.find(name);

	if (iter != wdays.cend())
		return iter->second;

	return 1/*SIZE_MAX*/;
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
			clines = json_array_size(json);

			bookmark_t elem = bookmark_t();

			size_t index;
			json_t* js_wobj;

			json_array_foreach(json, index, js_wobj) {

				if (!json_is_object(js_wobj)) break;

				json_t* js_fld;
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

					if (dmp_str && strlen(dmp_str) > 20) {
						// Www Mmm dd hh:mm:ss yyyy
						std::stringstream Stream(dmp_str);
						int Year, Day, Hour, Minute, Second;
						std::string StrAmPm;
						std::string StrWeekDay;
						std::string StrMonth;

						char ThrowAway;
						Stream >> StrWeekDay >> StrMonth >> Day;
						Stream >> Hour >> ThrowAway >> Minute >> ThrowAway >> Second;
						Stream >> Year;

						time_t rawtime;
						struct tm timeinfo = { 0 };
						timeinfo.tm_year = Year - 1900;;
						timeinfo.tm_mon = get_month_index(StrMonth);
						timeinfo.tm_mday = Day;
						timeinfo.tm_wday = get_wday_index(StrWeekDay);
						timeinfo.tm_hour = Hour;
						timeinfo.tm_min = Minute;
						timeinfo.tm_sec = Second;
						timeinfo.tm_isdst = -1;

						rawtime = mktime(&timeinfo);
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

				temp_data.push_back(elem);	//save to vector
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

		replaceMasterList(temp_data, masterList);
	}
	catch (...) {
		//..
	}

	return true;
}

