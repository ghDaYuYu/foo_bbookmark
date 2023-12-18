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

	const bookmark_t getDummy() {
		return dummy;
	}

	bool getDyna() {
		return dummy.isRadio() && dummy.dyna;
	}
	void setDyna(bool state) {
		dummy.dyna = dummy.isRadio() && state;
	}

	bool checkDummyIsRadio() {
		return (bool)dummy.isRadio();
	}

	bool checkDummyIsRadio(const pfc::string8 path) {
		return (bool)dummy.isRadio(path);
	}
	
	bool fetchHelloRadioStationName(pfc::string8 &out);

	bool CheckAutoFilter();

	void updateDummyTime();
	void updateDummy();
	bool upgradeDummy(std::list< dlg::CListControlBookmark*> guiList);

	void ResetRestoredDummy();
	void SetRestoredDummy(bookmark_t& bm);
	void checkDeletedRestoredDummy(const bit_array& mask, size_t count);

	void resetDummyKeepDyna() {
		auto tmp = dummy;
		resetDummyAll();
		dummy.dyna = tmp.dyna;
		dummy.desc = tmp.desc;
		dummy.comment = tmp.comment;
	}

	void resetDummyLocChecks() {
		dummy.need_loc_retries = 0;
		dummy.need_playlist = true;
	}

	void resetDummyAll() { dummy.reset(); }
	void setDummyTime(double time) { dummy.set_time(time); }

	bool isRestoredDummy(const bookmark_t& bm);
	bool isRestoredRadioDummy(const bookmark_t& bm);

	void refresh_ui(bool bselect, bool bensure_visible, const std::vector<bookmark_t>& masterList, std::list< dlg::CListControlBookmark*> guiLists);

private:

	bookmark_t dummy;
	bookmark_t restored_dummy;
	bool m_updatePlaylist = true;

	titleformat_object::ptr m_pttf_title = nullptr;
};

#pragma warning( push )
#pragma warning( disable:4996 )

inline void gimme_date(bookmark_t& out) {

	pfc::string8 date;
	pfc::string8 runtime_date;

	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	auto sctime = asctime(&tm);

	date.set_string(sctime);
	date.truncate_last_char();

		char buffer[DATE_BUFFER_SIZE];
		std::strftime(buffer, DATE_BUFFER_SIZE, cfg_date_format.get(), &tm);
		out.runtime_date = buffer;
		out.date = date;
}
#pragma warning( pop )
