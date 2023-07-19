#pragma once

#include "bookmark_types.h"

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
		bool m_updatePlaylist = true;

	public:

		bookmark_automatic() {
			//..
		};

		~bookmark_automatic() {
			//..
		};

		bool checkDummy() {
			return (bool)dummy.desc.get_length();
		}

		void updateDummyTime();
		void updateDummy();
		bool upgradeDummy(std::vector<bookmark_t>& masterList, std::list< dlg::CListControlBookmark*> guiList);
	};

#pragma warning( push )
#pragma warning( disable:4996 )
	inline void gimme_time(pfc::string8& out) {

		auto t = std::time(nullptr);
		auto tm = *std::localtime(&t);

		auto sctime = asctime(&tm);

		out.set_string(sctime);
		out.truncate_last_char();
	}
#pragma warning( pop )
