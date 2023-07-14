#pragma once

#include "bookmark_types.h"

class bookmark_persistence {
private:
	const bool noisy = true;
public:
	bookmark_persistence();
	~bookmark_persistence();

	//Stores the contents of g_masterList in a persistent file
	void writeDataFile(std::vector<bookmark_t> & masterList);

	//Replaces the contents of g_masterList with the contents of the persistent file
	BOOL readDataFile(std::vector<bookmark_t> & masterList);

private:
	static void replaceMasterList(std::vector<bookmark_t>& newContent, std::vector<bookmark_t>& masterList);
	std::vector<pfc::string8> splitString(const char * str, char separator);

	pfc::string8 genFilePath();
};

