#pragma once

#include <guiddef.h>
#include "pfc/int_types.h"
#include "pfc/string-lite.h"

	inline constexpr double KMin_Time = 1.5;

	struct bookmark_t {
	private:
		double time = 0.0;
	public:
		pfc::string8 desc;
		pfc::string8 playlist;
		GUID guid_playlist = pfc::guid_null;
		pfc::string8 path;
		t_uint32 subsong = 0;
		pfc::string8 comment;
		pfc::string8 date;
		bool need_playlist = false;
		void set_rt_time(double t) {
			time = t > KMin_Time ? t : 0.0;
		}
		void set_time(double t) {
			time = t > KMin_Time ? t : 0.0;
		}
		const double get_time() const { return time; }
	};
