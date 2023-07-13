#pragma once

	//====================DATA TYPES===================
	struct bookmark_t {
		double m_time;
		pfc::string8 m_desc;
		pfc::string8 m_playlist;
		GUID m_guid_playlist;
		pfc::string8 m_path;
		uint32_t m_subsong;
	};