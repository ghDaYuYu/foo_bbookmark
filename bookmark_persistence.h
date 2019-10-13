#pragma once

#include <vector>
#include <list>

#include "CListControlBookmark.h"
#include "bookmark_types.h"

class bookmark_persistence {
private:
	const bool noisy = false;
public:
	bookmark_persistence();
	~bookmark_persistence();

	//Stores the contents of g_masterList in a persistent file
	void write(std::vector<bookmark_t> & masterList);

	//replaces the contents of g_masterList with the contents of the persistent file
	BOOL readDataFile(std::vector<bookmark_t> & masterList);

private:
	static void replaceMasterList(std::vector<bookmark_t>& newContent, std::vector<bookmark_t>& masterList);

	pfc::string genFilePath();
};

