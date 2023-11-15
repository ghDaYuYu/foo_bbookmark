#pragma once

#include "bookmark_types.h"
#include "bookmark_preferences.h"

#include <vector>
#include <list>
#include <sstream>
#include <iomanip>

namespace dlg {

	class CListControlBookmark;

}

class bookmark_automatic {

private:

	bookmark_t dummy;
	bookmark_t restored_dummy;
	bool m_updatePlaylist = true;

public:

	bookmark_automatic() {
		//..
	};

	~bookmark_automatic() {
		//..
	}

	bool checkDummy() {
		return (bool)dummy.desc.get_length();
	}

	bool checkDummyIsRadio() {
		return (bool)dummy.isRadio();
	}

	bool CheckAutoFilter();

	void updateDummyTime();
	void updateDummy();
	bool upgradeDummy(std::vector<bookmark_t>& masterList, std::list< dlg::CListControlBookmark*> guiList);
	void updateRestoredDummy(bookmark_t& bm);

	void refresh_ui(bool bselect, bool bensure_visible, std::vector<bookmark_t>& masterList, std::list< dlg::CListControlBookmark*> guiLists);
};

#pragma warning( push )
#pragma warning( disable:4996 )

inline void gimme_time(bookmark_t& out) {

	pfc::string8 date;
	pfc::string8 runtime_date;

	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	auto sctime = asctime(&tm);

	date.set_string(sctime);
	date.truncate_last_char();

	char buffer[255];
	std::strftime(buffer, 255, cfg_date_format.get(), &tm);
	out.runtime_date = buffer;
	out.date = date;
}
#pragma warning( pop )
