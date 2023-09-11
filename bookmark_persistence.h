#pragma once
#include "helpers/CmdThread.h"
#include "bookmark_types.h"
#include <vector>

inline ThreadUtils::cmdThread cmdTh;

class bookmark_persistence {
private:
	const bool noisy = true;
public:
	bookmark_persistence();
	~bookmark_persistence();

	void writeDataFile(std::vector<bookmark_t>& masterList);
	//Stores the contents of g_masterList in a persistent file
	void writeDataFileJSON(std::vector<bookmark_t>& masterList);

	//Replaces the contents of g_masterList with the contents of the persistent file
	bool readDataFileJSON(std::vector<bookmark_t>& masterList);

private:
	static void replaceMasterList(std::vector<bookmark_t>& newContent, std::vector<bookmark_t>& masterList);

	pfc::string8 genFilePath();
};

