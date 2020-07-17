#pragma once

#include <sstream>
#include <vector>
#include <list>

#include "CListControlBookmark.h"
#include "bookmark_types.h"
#include "bookmark_preferences.h"

class bookmark_persistence {
private:
	const bool noisy = true;
public:
	bookmark_persistence();
	~bookmark_persistence();

	//Stores the contents of g_masterList in a persistent file
	void write(std::vector<bookmark_t> & masterList);

	//Replaces the contents of g_masterList with the contents of the persistent file
	BOOL readDataFile(std::vector<bookmark_t> & masterList);

private:
	static void replaceMasterList(std::vector<bookmark_t>& newContent, std::vector<bookmark_t>& masterList);
	std::vector<pfc::string> splitString(const char * str, char separator);

	pfc::string genFilePath();
};

