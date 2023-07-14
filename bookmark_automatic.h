#pragma once

#include "bookmark_types.h"
#include "bookmark_preferences.h"
#include "bookmark_list_control.h"

#include <vector>
#include <list>
#include <sstream>

class bookmark_automatic
{
private:
	bookmark_t dummy;
	bool m_updatePlaylist = true;
public:
	bookmark_automatic();
	~bookmark_automatic();

	bool checkDummy() {
		return (bool)dummy.m_desc.get_length();
	}
	void updateDummyTime();
	void updateDummy();
	bool upgradeDummy(std::vector<bookmark_t> & masterList, std::list<CListControlBookmark *> & guiLists);
};

