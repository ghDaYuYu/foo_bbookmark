#include "stdafx.h"

#include <string>
#include <sstream>

#include "bookmark_persistence.h"
#include "bookmark_preferences.h"

bookmark_persistence::bookmark_persistence() {
	//..
}

bookmark_persistence::~bookmark_persistence()
{
	//..
}

const char separator = '\v';

//Stores the contents of masterList in a persistent file
void bookmark_persistence::writeDataFile(std::vector<bookmark_t> & masterList) {
	if (core_api::is_quiet_mode_enabled()) {
		FB2K_console_print("Quiet mode, will not write bookmarks to file");
		return;
	}

	//Create or overwrite the file
	foobar2000_io::file_ptr file = foobar2000_io::fileOpenWriteNew(genFilePath().c_str(), fb2k::noAbort);

	try {
		if (cfg_verbose) FB2K_console_print("Preparing Data for write.");

		size_t n_entries = masterList.size();
		pfc::string8 linebreak = "\n";

		auto addValue = [](foobar2000_io::file_ptr file, pfc::string value, const char separator) {
			file->write_string_raw(value.c_str(), fb2k::noAbort);
			file->write_string_raw(&separator, fb2k::noAbort);
		};

		for (size_t i = 0; i < n_entries; i++) {
			//writeDataFile the time first
			addValue(file, std::to_string(masterList[i].m_time).c_str(), separator);
			//and then the description, playlist and path
			addValue(file, masterList[i].m_desc, separator);
			addValue(file, masterList[i].m_playlist, separator);
			addValue(file, pfc::print_guid(masterList[i].m_guid_playlist).c_str(), separator);
			addValue(file, masterList[i].m_path, separator);
			//lastly, add the subsong index
			addValue(file, std::to_string(masterList[i].m_subsong).c_str(), '\n');
		}
		file->set_eof(fb2k::noAbort);
		
		file->flushFileBuffers(fb2k::noAbort);
        file.release()
		
		FB2K_console_print("Wrote bookmarks to file"); //, genFilePath().c_str();
	} catch (foobar2000_io::exception_io e) {
		console::complain("Could not write bookmarks to file", e);
	} catch (...) {
		console::complain("Could not write bookmarks to file", "Unhandled Exception");
	}
}

//replaces the contents of masterList with the contents of the persistent file
BOOL bookmark_persistence::readDataFile(std::vector<bookmark_t>& masterList) {

	const char separator = '\v';

	size_t clines = 0;
	std::vector<bookmark_t> temp_data;
	FB2K_console_print("Reading basic bookmarks from file");
	try {
		//std::fstream m_dat_file;

		foobar2000_io::file_ptr file = foobar2000_io::fileOpenReadExisting(genFilePath().c_str(), fb2k::noAbort);

		if (file->is_eof(fb2k::noAbort))
			return false; //file empty

		pfc::string_formatter fullContent;
		file->read_string_raw(fullContent, fb2k::noAbort);

		std::vector<pfc::string8> lines = splitString(fullContent, '\n');
		clines = lines.size();
		//uint32_t subsong;
		for (pfc::string8 line : lines) {
			if (cfg_verbose) FB2K_console_print("Found line: ", line.c_str());

			if (line.get_length() == 0)
				continue;

			auto values = splitString(line.c_str(), separator);

			if (values.size() < SZ_DAT_VER_1) {
				FB2K_console_print("Insufficient values in line, skipping.\nLine was:", line.c_str());
				continue;
			}
			else {
				if (values.size() < SZ_DAT_VER_2) {
					//convert to VER_2 (no backwards compatibility)
					pfc::string8 str_plguid;
					if (core_version_info_v2::get()->test_version(2, 0, 0, 0)) {
						size_t ndx = playlist_manager_v5::get()->find_playlist(values[2]);
						if (ndx != pfc_infinite) {
							GUID plguid = playlist_manager_v5::get()->playlist_get_guid(ndx);
							str_plguid = pfc::print_guid(plguid);
						}
					}
					//insert V2 guid_playlist after playlist ndx
					values.insert(values.begin() + 3, str_plguid);
				}
			}

			//construct bookmark_t
			bookmark_t elem = bookmark_t();
			elem.m_time = pfc::string_to_float(values[0].c_str(), values[0].get_length());
			if (cfg_verbose) console::formatter() << "Read time: " << elem.m_time;
			elem.m_desc = values[1];
			if (cfg_verbose) console::formatter() << "Read desc" << elem.m_desc.c_str();
			elem.m_playlist = values[2];
			if (cfg_verbose) console::formatter() << "Read playlist name" << elem.m_playlist.c_str();
			elem.m_guid_playlist = pfc::GUID_from_text(values[3]);
			if (cfg_verbose) console::formatter() << "Read playlist GUID" << pfc::print_guid(elem.m_guid_playlist).c_str();
			elem.m_path = values[4];
			if (cfg_verbose) console::formatter() << "Read path" << elem.m_path.c_str();
			elem.m_subsong = atoi(values[5].c_str()); // pfc::string_to_float(values[5].c_str(), values[5].get_length());
			if (cfg_verbose) console::formatter() << "Read subsong" << elem.m_subsong;

			temp_data.push_back(elem);	//save to vector
		}

		//file->close() TODO: find out equivalent
		file.release();

		if (cfg_verbose) {
			console::formatter() << "file content:";
			for (size_t i = 0; i < temp_data.size(); ++i)
				FB2K_console_print("time ", i, ": ", temp_data[i].m_time);
		}
	}
	catch (foobar2000_io::exception_io e) {
		console::complain("Reading data from file failed", e);
		return false;
	}
	catch (...) {
		console::complain("Reading data from file failed", "Unhandled Exception");
		return false;
	}


	FB2K_console_print("Read basic bookmarks from file");

	//actually emplace data
	replaceMasterList(temp_data, masterList);

	return true;
}

//Stores the contents of masterList in a persistent file
void bookmark_persistence::writeDataFile(std::vector<bookmark_t> & masterList) {

	if (core_api::is_quiet_mode_enabled()) {
		FB2K_console_print("Quiet mode, will not write bookmarks to file");
		return;
	}

	const char separator = '\v';

	//Create or overwrite the file
	foobar2000_io::file_ptr file = foobar2000_io::fileOpenWriteNew(genFilePath().c_str(), fb2k::noAbort);

	try {
		if (cfg_verbose) FB2K_console_print("COMPONENT_NAME_H << ": preparing to write data file.");

		size_t n_entries = masterList.size();
		pfc::string8 linebreak = "\n";

		auto addValue = [&](foobar2000_io::file_ptr file, pfc::string8 value, const char separator) {
			file->write_string_raw(value.c_str(), fb2k::noAbort);
			file->write_string_raw(&separator, fb2k::noAbort);
		};

		for (size_t i = 0; i < n_entries; i++) {
			//writeDataFile the time first
			addValue(file, std::to_string(masterList[i].m_time).c_str(), separator);
			//and then the description, playlist and path
			addValue(file, masterList[i].m_desc, separator);
			addValue(file, masterList[i].m_playlist, separator);
			addValue(file, pfc::print_guid(masterList[i].m_guid_playlist).c_str(), separator);
			addValue(file, masterList[i].m_path, separator);
			addValue(file, std::to_string(masterList[i].m_subsong).c_str(), '\n');	//lastly, add the subsong index
		}
		file->set_eof(fb2k::noAbort);

		file->flushFileBuffers(fb2k::noAbort);
		file.release();

		FB2K_console_print("COMPONENT_NAME_H << ": wrote bookmarks to file");
	} catch (foobar2000_io::exception_io e) {
		FB2K_console_print("Could not write bookmarks to file", e);
	} catch (...) {
		FB2K_console_print("Could not write bookmarks to file", "Unhandled Exception");
	}
}

void bookmark_persistence::replaceMasterList(std::vector<bookmark_t>& newContent, std::vector<bookmark_t>& masterList) {
	if (cfg_verbose) console::formatter() << COMPONENT_NAME_H << ": replacing cache";

	masterList.clear();
	masterList.insert(masterList.begin(), newContent.begin(), newContent.end());
}

//todo: utils
std::vector<pfc::string8> bookmark_persistence::splitString(const char * str, char separator) {
	std::vector<pfc::string8> parts;
	std::stringstream ss(str);
	std::string token;
	while (std::getline(ss, token, separator)) {
		parts.push_back(token.c_str());
	}
	return parts;
}

pfc::string8 bookmark_persistence::genFilePath() {
	static std::string path;

	if (path.empty()) {
		path = std::string(core_api::get_profile_path()).append("\\configuration\\").append(core_api::get_my_file_name()).append(".dll.dat").substr(7, std::string::npos);
	}
	//use configuration subdir, add own name, add .dll.dat, remove leading file://
	return pfc::string8(path.c_str());
}

