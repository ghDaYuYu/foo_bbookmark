#include "stdafx.h"
#include <map>
#include <sstream>
#include "bookmark_persistence.h"
#include "bookmark_types.h"


int get_wday_index(std::string name)
{
	std::map<std::string, int> wdays
	{
			{ "Mon", 0 },
			{ "Tue", 1 },
			{ "Wed", 2 },
			{ "Thu", 3 },
			{ "Fri", 4 },
			{ "Sat", 5 },
			{ "Sun", 6 },
	};

	const auto iter = wdays.find(name);

	if (iter != wdays.cend())
		return iter->second;
	return 1/*SIZE_MAX*/;
}

int get_month_index(std::string name)
{
	std::map<std::string, int> months
	{
			{ "Jan", 0 },
			{ "Feb", 1 },
			{ "Mar", 2 },
			{ "Apr", 3 },
			{ "May", 4 },
			{ "Jun", 5 },
			{ "Jul", 6 },
			{ "Aug", 7 },
			{ "Sep", 8 },
			{ "Oct", 9 },
			{ "Nov", 10 },
			{ "Dec", 11 }
	};

	const auto iter = months.find(name);

	if (iter != months.cend())
		return iter->second;
	return 1/*SIZE_MAX*/;
}

inline void unix_str_date_to_time(pfc::string8 unix_date, time_t& out_rawtime, tm& out_tm) {

	int Year, Day, Hour, Minute, Second;
	std::string StrAmPm;
	std::string StrWeekDay;
	std::string StrMonth;

	char ThrowAway;

	std::stringstream Stream(unix_date.c_str());
	Stream >> StrWeekDay >> StrMonth >> Day;
	Stream >> Hour >> ThrowAway >> Minute >> ThrowAway >> Second;
	Stream >> Year;

	out_tm.tm_year = Year - 1900;
	out_tm.tm_mon = get_month_index(StrMonth);
	out_tm.tm_mday = Day;
	out_tm.tm_wday = get_wday_index(StrWeekDay);
	out_tm.tm_hour = Hour;
	out_tm.tm_min = Minute;
	out_tm.tm_sec = Second;
	out_tm.tm_isdst = -1;

	out_rawtime = mktime(&out_tm);
}