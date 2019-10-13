#include "stdafx.h"
#include "bookmark_persistence.h"


bookmark_persistence::bookmark_persistence() {

}

#define AB_C_DUMMY foobar2000_io::abort_callback_dummy()

//Stores the contents of masterList in a persistent file
void bookmark_persistence::write(std::vector<bookmark_t> & masterList) {
	if (core_api::is_quiet_mode_enabled()) {
		console::formatter() << "Quiet mode, will not write bookmarks to file";
		return;
	}

	//Create or overwrite the file
	foobar2000_io::file_ptr file = foobar2000_io::fileOpenWriteNew(genFilePath().c_str(), AB_C_DUMMY);

	//if (!m_dat_file.is_open())
	//	m_dat_file.open(genFilePath().c_str(), std::ios::in | std::ios::out | std::ios::binary);

	//if (m_dat_file.fail()) {//File may not exist
	//	console::formatter() << "Attempting to create nonexistent file";
	//	m_dat_file.close();
	//	m_dat_file.open(genFilePath().c_str(), std::ios::binary | std::ios::out);
	//	m_dat_file.close();
	//	m_dat_file.open(genFilePath().c_str(), std::ios::in | std::ios::binary | std::ios::out);
	//}

	try {
		//console::formatter() << "Preparing Data for write.";

		size_t n_entries = masterList.size();

		file->write(&n_entries, sizeof(n_entries), AB_C_DUMMY);

		//console::formatter() << "wrote number of datatriplets: " << n_entries;
		for (size_t i = 0; i < n_entries; i++) {
			file->write(&masterList[i].m_time, sizeof(double), AB_C_DUMMY);	//write the time first
			//and then the description, playlist and path
			file->write_string(masterList[i].m_desc, AB_C_DUMMY);
			file->write_string(masterList[i].m_playlist, AB_C_DUMMY);
			file->write_string(masterList[i].m_path, AB_C_DUMMY);
			file->write(&masterList[i].m_subsong, sizeof(uint32_t), AB_C_DUMMY);	//lastly, add the subsong index
		}
		file->set_eof(AB_C_DUMMY);
		console::formatter() << "Wrote bookmarks to file"; // << genFilePath().c_str();
	}
	catch (foobar2000_io::exception_io e) {
		console::complain("Could not write bookmarks to file", e);
	}
	catch (...) {
		console::complain("Could not write bookmarks to file", "Unhandled Exception");
	}
}

//replaces the contents of masterList with the contents of the persistent file
BOOL bookmark_persistence::readDataFile(std::vector<bookmark_t> & masterList) {
	std::vector<bookmark_t> temp_data;
	console::formatter() << "Reading basic bookmarks from file";
	try {
		//std::fstream m_dat_file;

		foobar2000_io::file_ptr file = foobar2000_io::fileOpenReadExisting(genFilePath().c_str(), AB_C_DUMMY);

		//read size to read
		size_t size;		
		if (file->is_eof(AB_C_DUMMY))	
			return false; //file empty
		file->read(&size, sizeof(size), AB_C_DUMMY);

		if (noisy) console::formatter() << "read a size: " << size;
		temp_data.reserve(size);
		//m_dat_file.seekg(sizeof(size));

		//Read $size values from file
		double d;
		uint32_t subsong;

		for (size_t i = 0; i < size; i++) {
			file->read((char*)&d, sizeof(double), AB_C_DUMMY);	//read the time
			if (noisy) console::formatter() << "Read a time: " << d;

			
			pfc::string description = file->read_string(AB_C_DUMMY);
			if (noisy) console::formatter() << "Read a description: " << description.c_str();

			pfc::string playlistName = file->read_string(AB_C_DUMMY);
			if (noisy) console::formatter() << "Read a playlistName: " << playlistName.c_str();

			pfc::string path = file->read_string(AB_C_DUMMY);
			if (noisy) console::formatter() << "Read a path: " << path.c_str();

			file->read((char*)&subsong, sizeof(uint32_t), AB_C_DUMMY);	//read the subsong
			if (noisy) console::formatter() << "Read a subsong index: " << subsong;

			//construct bookmark_t
			bookmark_t elem = bookmark_t();
			elem.m_time = d;
			if (noisy) console::formatter() << "Pushing desc";
			elem.m_desc = description;
			if (noisy) console::formatter() << "Pushing plName";
			elem.m_playlist = playlistName;
			elem.m_path = path;
			elem.m_subsong = subsong;

			temp_data.push_back(elem);	//save to vector
		}

		//file->close() TODO: find out equivalent

		if (noisy) {
			console::formatter() << "file content:";
			for (size_t i = 0; i < temp_data.size(); ++i)
				console::formatter() << "time " << i << ": " << temp_data[i].m_time;
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


	console::formatter() << "Read basic bookmarks from file";

	//actually emplace data
	replaceMasterList(temp_data, masterList);

	return true;
}



void bookmark_persistence::replaceMasterList(std::vector<bookmark_t>  &newContent, std::vector<bookmark_t> & masterList) {
	//console::formatter() << "Basic Bookmarks: Replacing Cache";

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
