#include "stdafx.h"
#include "bookmark_persistence.h"


bookmark_persistence::bookmark_persistence() {

}

const char separator = '\v';

//Stores the contents of masterList in a persistent file
void bookmark_persistence::write(std::vector<bookmark_t> & masterList) {
	if (core_api::is_quiet_mode_enabled()) {
		FB2K_console_print("Quiet mode, will not write bookmarks to file");
		return;
	}

	//Create or overwrite the file
	foobar2000_io::file_ptr file = foobar2000_io::fileOpenWriteNew(genFilePath().c_str(), fb2k::noAbort);

	try {
		if (cfg_bookmark_verbose) FB2K_console_print("Preparing Data for write.");

		size_t n_entries = masterList.size();
		pfc::string linebreak = "\n";

		auto addValue = [](foobar2000_io::file_ptr file, pfc::string value, const char separator) {
			file->write_string_raw(value.c_str(), fb2k::noAbort);
			file->write_string_raw(&separator, fb2k::noAbort);
		};

		for (size_t i = 0; i < n_entries; i++) {
			//write the time first
			addValue(file, pfc::format_float(masterList[i].m_time), separator);
			//and then the description, playlist and path
			addValue(file, masterList[i].m_desc, separator);
			addValue(file, masterList[i].m_playlist, separator);
			addValue(file, masterList[i].m_path, separator);
			addValue(file, pfc::format_float(masterList[i].m_subsong).c_str(), '\n');	//lastly, add the subsong index			
		}
		file->set_eof(fb2k::noAbort);
		FB2K_console_print("Wrote bookmarks to file"); //, genFilePath().c_str();
	} catch (foobar2000_io::exception_io e) {
		console::complain("Could not write bookmarks to file", e);
	} catch (...) {
		console::complain("Could not write bookmarks to file", "Unhandled Exception");
	}
}

//replaces the contents of masterList with the contents of the persistent file
BOOL bookmark_persistence::readDataFile(std::vector<bookmark_t> & masterList) {
	std::vector<bookmark_t> temp_data;
	FB2K_console_print("Reading basic bookmarks from file");
	try {
		//std::fstream m_dat_file;

		foobar2000_io::file_ptr file = foobar2000_io::fileOpenReadExisting(genFilePath().c_str(), fb2k::noAbort);

		if (file->is_eof(fb2k::noAbort))
			return false; //file empty


		pfc::string_formatter fullContent;
		file->read_string_raw(fullContent, fb2k::noAbort);

		std::vector<pfc::string> lines = splitString(fullContent, '\n');
		for (pfc::string line : lines) {
			if (cfg_bookmark_verbose) FB2K_console_print("Found line: ", line.c_str());

			if (line.get_length() == 0)
				continue;

			auto values = splitString(line.c_str(), separator);

			if (values.size() < 5) {
				FB2K_console_print("Insufficient values in line, skipping.\nLine was:", line.c_str());
				continue;
			}

			//construct bookmark_t
			bookmark_t elem = bookmark_t();
			elem.m_time = pfc::string_to_float(values[0].c_str(), values[0].get_length());
			if (cfg_bookmark_verbose) FB2K_console_print("Read a time: ", elem.m_time);
			elem.m_desc = values[1];
			if (cfg_bookmark_verbose) FB2K_console_print("Read desc", elem.m_desc.c_str());
			elem.m_playlist = values[2];
			if (cfg_bookmark_verbose) FB2K_console_print("Read plName", elem.m_playlist.c_str());
			elem.m_path = values[3];
			if (cfg_bookmark_verbose) FB2K_console_print("Read path", elem.m_path.c_str());
			elem.m_subsong = pfc::string_to_float(values[4].c_str(), values[4].get_length());
			if (cfg_bookmark_verbose) FB2K_console_print("Read subsong", elem.m_subsong);


			temp_data.push_back(elem);	//save to vector
		}

		//file->close() TODO: find out equivalent

		if (cfg_bookmark_verbose) {
			FB2K_console_print("file content:");
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


std::vector<pfc::string> bookmark_persistence::splitString(const char * str, char separator) {
	std::vector<pfc::string> parts;
	std::stringstream ss(str);
	std::string token;
	while (std::getline(ss, token, separator)) {
		parts.push_back(pfc::string(token.c_str()));
	}
	return parts;
}

void bookmark_persistence::replaceMasterList(std::vector<bookmark_t>  &newContent, std::vector<bookmark_t> & masterList) {
	if (cfg_bookmark_verbose) FB2K_console_print("Basic Bookmarks: Replacing Cache");

	masterList.clear();
	masterList.reserve(newContent.size());

	for (size_t walk = 0; walk < newContent.size(); ++walk) {
		masterList.emplace_back(newContent[walk]);	//copy into data
	}
}

pfc::string bookmark_persistence::genFilePath() {
	static std::string path;

	if (path.empty()) {
		path = std::string(core_api::get_profile_path()).append("\\configuration\\").append(core_api::get_my_file_name()).append(".dll.dat").substr(7, std::string::npos);
	}
	//use configuration subdir, add own name, add .dll.dat, remove leading file://
	return pfc::string(path.c_str());
}

bookmark_persistence::~bookmark_persistence()
{
}
