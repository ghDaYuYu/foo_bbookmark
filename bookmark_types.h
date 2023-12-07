#pragma once

#include <guiddef.h>
#include "pfc/int_types.h"
#include "pfc/string-lite.h"

#define DATE_BUFFER_SIZE 255
#define LOC_RETRIES 2

	inline static const int kUI_CONF_VER = 1;
	inline constexpr double KMin_Lapse = 2.0;

	struct bookmark_t {
	private:
		double time = 0.0;
	public:
		pfc::string8 path;
		pfc::string8 desc;
		pfc::string8 comment;
		pfc::string8 date;
		pfc::string8 runtime_date;
		t_uint32 subsong = 0;

		bool need_playlist = true;
		t_uint32 need_loc_retries = 0;

		bool dyna = false;

		pfc::string8 playlist;
		GUID guid_playlist = pfc::guid_null;

		void set_rt_time(double t) {
			time = t < KMin_Lapse ? 0.0 : t;
		}
		void set_time(double t) {
			if (t == SIZE_MAX) {
				t = 0.0;
				return;
			}
			time = t < KMin_Lapse ? 0.0 : t;
		}
		const double get_time() const {
			return time;
		}

		bool isRadio() const {
			return isRadio(path);
		}

		//todo
		bool isRadio(const pfc::string8 path) const {
			return path.startsWith("https://");
		}

		void reset() {
			*this = bookmark_t();
		}
	};
