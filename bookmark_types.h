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
		pfc::string8 name;

	public:

		GUID guid_bm = pfc::guid_null;
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

		const pfc::string8 get_name(bool or_desc) const {
			return name.get_length() ? name : or_desc ? desc : "";
		}

		void set_name(pfc::string8 aname) {
			name = aname;
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
			this->guid_bm = pfc::createGUID();
		}

		void swap(bookmark_t& other)
		{
			bookmark_t tmp = other;

			other.guid_bm = this->guid_bm;
			other.comment.move(this->comment);
			other.playlist.move(this->playlist);
			other.guid_playlist = this->guid_playlist;
			other.date.move(this->date);
			other.runtime_date.move(this->runtime_date);
			other.name.move(this->name);
			other.desc.move(this->desc);
			other.path.move(this->path);
			other.subsong = this->subsong;
			other.time = this->time;

			guid_bm = tmp.guid_bm;
			comment.move(tmp.comment);
			playlist.move(tmp.playlist);
			guid_playlist = tmp.guid_playlist;
			date.move(tmp.date);
			runtime_date.move(tmp.runtime_date);
			name.move(tmp.name);
			desc.move(tmp.desc);
			path.move(tmp.path);
			subsong = tmp.subsong;
			time = tmp.time;
		}
	};

	extern void unix_str_date_to_time(pfc::string8 unix_date, time_t& out_rawtime, tm& out_tm);

	extern int get_month_index(std::string name);
	extern int get_wday_index(std::string name);
