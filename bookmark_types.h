#pragma once

#include <guiddef.h>
#include "pfc/int_types.h"
#include "pfc/string-lite.h"

	struct bookmark_t {
		double time = 0.0;
		pfc::string8 desc;
		pfc::string8 playlist;
		GUID guid_playlist = pfc::guid_null;
		pfc::string8 path;
		//todo:
		t_uint32 /*float*/ subsong = 0;
		pfc::string8 comment;
		pfc::string8 date;
	};
