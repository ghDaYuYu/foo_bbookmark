#include "stdafx.h"
#include "resource.h"
#include <helpers/atl-misc.h>

#include "bookmark_preferences.h"

#include <libPPUI/wtl-pp.h> // CCheckBox

#include <vector>
#include <list>
#include <sstream>

static const int stringlength = 256;

//---GUIDs---
static const GUID guid_cfg_bookmark_desc_format = { 0xb1cd9cbe, 0x4ee3, 0x4716, { 0x81, 0x7e, 0xbf, 0x5a, 0x91, 0x8b, 0x4c, 0x60 } };
static const GUID guid_cfg_bookmark_autosave_newTrack_playlists = { 0x7928d881, 0xbc00, 0x44d7, { 0xbb, 0x8b, 0xd6, 0xa7, 0x1, 0x2b, 0x91, 0xa8 } };

static const GUID guid_cfg_bookmark_autosave_onClose = { 0x5d055347, 0xd9a9, 0x4829, { 0xb7, 0xc9, 0x62, 0x0, 0x76, 0x19, 0x10, 0xbc } };
static const GUID guid_cfg_bookmark_autosave_newTrack = { 0xa687aaff, 0xf382, 0x473e, { 0xbf, 0x64, 0xf1, 0x4d, 0x30, 0x7, 0xf1, 0x45 } };
static const GUID guid_cfg_bookmark_autosave_newTrackFilter = { 0x781424f7, 0x9ff9, 0x4f7a, { 0x8a, 0x3d, 0xe9, 0x1d, 0x75, 0xd2, 0x17, 0xb0 } };

static const GUID guid_cfg_bookmark_verbose = { 0xe8342c32, 0xa1ac, 0x4c7d, { 0xbc, 0x20, 0x9, 0xec, 0x37, 0xab, 0x79, 0x66 } };

//--defaults---
static const pfc::string8 default_cfg_bookmark_desc_format = "%title%";
static const pfc::string8 default_cfg_bookmark_autosave_newTrack_playlists = "Podcatcher";

static const bool default_cfg_bookmark_autosave_newTrack = false;
static const bool default_cfg_bookmark_autosave_newTrackFilter = true;
static const bool default_cfg_bookmark_autosave_onClose = false;

static const bool default_cfg_bookmark_verbose = false;

// ---CFG_VARS---
cfg_string cfg_bookmark_desc_format(guid_cfg_bookmark_desc_format, default_cfg_bookmark_desc_format.c_str());
cfg_string cfg_bookmark_autosave_newTrack_playlists(guid_cfg_bookmark_autosave_newTrack_playlists, default_cfg_bookmark_autosave_newTrack_playlists.c_str());

cfg_bool cfg_bookmark_autosave_newTrack(guid_cfg_bookmark_autosave_newTrack, default_cfg_bookmark_autosave_newTrack);
cfg_bool cfg_bookmark_autosave_newTrackFilter(guid_cfg_bookmark_autosave_newTrackFilter, default_cfg_bookmark_autosave_newTrackFilter);
cfg_bool cfg_bookmark_autosave_onQuit(guid_cfg_bookmark_autosave_onClose, default_cfg_bookmark_autosave_onClose);

cfg_bool cfg_bookmark_verbose(guid_cfg_bookmark_verbose, default_cfg_bookmark_verbose);

struct boxAndBool_t {
	int idc;
	cfg_bool* cfg;
	bool def;
};

struct ectrlAndString_t {
	int idc;
	cfg_string* cfg;
	pfc::string8 def;
};



class CBookmarkPreferences : public CDialogImpl<CBookmarkPreferences>, public preferences_page_instance {
public:
	CBookmarkPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}
	std::vector<pfc::string8> m_currentPlNames;

	//dialog resource ID
	enum { IDD = IDD_BOOKMARK_PREFERENCES };
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP_EX(CBookmarkPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_CODE_HANDLER_EX(EN_CHANGE, OnEditChange)
		COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnCheckChange)
		END_MSG_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT uNotifyCode, int nId, CWindow wndCtl);
	void OnCheckChange(UINT uNotifyCode, int nId, CWindow wndCtl);
	bool HasChanged();
	void OnChanged();

	//TODO: group all these, then use for loops
	ectrlAndString_t eat_format = { IDC_TITLEFORMAT, &cfg_bookmark_desc_format, default_cfg_bookmark_desc_format };
	ectrlAndString_t eat_as_newTrackPlaylists = { IDC_AUTOSAVE_TRACK_FILTER, &cfg_bookmark_autosave_newTrack_playlists, default_cfg_bookmark_autosave_newTrack_playlists };

	boxAndBool_t bab_as_newTrack = { IDC_AUTOSAVE_TRACK, &cfg_bookmark_autosave_newTrack, default_cfg_bookmark_autosave_newTrack };
	boxAndBool_t bab_as_newTrackFilter = { IDC_AUTOSAVE_TRACK_FILTER_CHECK, &cfg_bookmark_autosave_newTrackFilter, default_cfg_bookmark_autosave_newTrackFilter };
	boxAndBool_t bab_as_exit = { IDC_AUTOSAVE_EXIT, &cfg_bookmark_autosave_onQuit, default_cfg_bookmark_autosave_onClose };

	boxAndBool_t bab_verbose = { IDC_VERBOSE, &cfg_bookmark_verbose, default_cfg_bookmark_verbose };

	void cfgToUi(boxAndBool_t bab) {
		CCheckBox cb(GetDlgItem(bab.idc));
		cb.SetCheck(*(bab.cfg));
	}
	void defToUi(boxAndBool_t bab) {
		CCheckBox cb(GetDlgItem(bab.idc));
		cb.SetCheck(bab.def);
	}
	void uiToCfg(boxAndBool_t bab) {
		CCheckBox cb(GetDlgItem(bab.idc));
		*(bab.cfg) = (bool)cb.GetCheck();
	}
	bool isUiChanged(boxAndBool_t bab) {
		CCheckBox cb(GetDlgItem(bab.idc));
		return *(bab.cfg) != (bool)cb.GetCheck();
	}

	void cfgToUi(ectrlAndString_t eat) {
		pfc::string8 cfgS = *(eat.cfg);
		wchar_t wideS[stringlength];
		size_t outSize;
		mbstowcs_s(&outSize, wideS, stringlength, cfgS.c_str(), stringlength - 1);
		SetDlgItemText(eat.idc, wideS);
	}
	void defToUi(ectrlAndString_t eat) {
		wchar_t wideString[stringlength];
		size_t outSize;
		mbstowcs_s(&outSize, wideString, stringlength, eat.def.c_str(), stringlength - 1);
		SetDlgItemText(eat.idc, wideString);
	}
	void uiToCfg(ectrlAndString_t eat) {
		//Convert editBox content into a cfg_string compatible format
		TCHAR fieldContent[stringlength];
		GetDlgItemTextW(eat.idc, (LPTSTR)fieldContent, stringlength);

		char convertedContent[stringlength];
		size_t outSize;
		wcstombs_s(&outSize, convertedContent, fieldContent, stringlength - 1);

		*eat.cfg = convertedContent;
	}
	bool isUiChanged(ectrlAndString_t eat) {
		TCHAR fieldContent[stringlength];
		GetDlgItemTextW(eat.idc, (LPTSTR)fieldContent, stringlength);

		char convertedContent[stringlength];
		size_t outSize;
		wcstombs_s(&outSize, convertedContent, fieldContent, stringlength - 1);

		return strcmp(convertedContent, *eat.cfg);
	}

	static_api_ptr_t<playback_control> m_playback_control;
	const preferences_page_callback::ptr m_callback;

};



BOOL CBookmarkPreferences::OnInitDialog(CWindow wndCtl, LPARAM) {
	//Push cfg_vars to UI
	cfgToUi(eat_format);
	cfgToUi(eat_as_newTrackPlaylists);

	cfgToUi(bab_as_exit);
	cfgToUi(bab_as_newTrack);
	cfgToUi(bab_as_newTrackFilter);

	cfgToUi(bab_verbose);

	//populate the combo box with all currently existing playlists
	size_t plCount = playlist_manager::get()->get_playlist_count();
	m_currentPlNames.clear();
	//m_currentPlNames.resize(plCount);
	CComboBox comboBox = GetDlgItem(IDC_COMBO1);
	for (size_t i = 0; i < plCount; i++) {
		//get playlist name
		pfc::string8 plName;
		playlist_manager::get()->playlist_get_name(i, plName);
		console::formatter() << "found playlist called " << plName.c_str();
		m_currentPlNames.emplace_back(plName);

		//convert pl name
		wchar_t wideString[stringlength];
		size_t outSize;
		mbstowcs_s(&outSize, wideString, stringlength, plName.c_str(), stringlength - 1);

		//write pl name
		SendMessage(comboBox, CB_ADDSTRING, 0, (LPARAM)wideString);
	}
	SendMessage(comboBox, CB_SETCURSEL, 0, 0);
	

	comboBox.SetTopIndex(0);

	return FALSE;
}

void CBookmarkPreferences::OnEditChange(UINT uNotifyCode, int nId, CWindow wndCtl) {
	// not much to do here
	OnChanged();
}

void CBookmarkPreferences::OnCheckChange(UINT uNotifyCode, int nId, CWindow wndCtl) {
	if (nId == IDC_BUTTON1) {
		//Todo: add currently selected playlist to the filter edit control
		CComboBox comboBox = GetDlgItem(IDC_COMBO1);
		int selected = comboBox.GetCurSel();

		if (selected >= 0 && selected < m_currentPlNames.size()) {
			pfc::string8 newName;
			newName += m_currentPlNames[selected];
			//replace all commas with dots (because of the comma-seperated list)
			newName.replace_char(',', '.', 0);

			//check if the cfg already contains this name
			std::stringstream ss(cfg_bookmark_autosave_newTrack_playlists.c_str());
			std::string token;
			while (std::getline(ss, token, ',')) {
				if (strcmp(token.c_str(), newName.c_str()) == 0) {
					//We already know that name, fizzle 
					return;
				}
			}

			console::formatter() << "adding to auto-bookmarking playlists: " << newName;

			//Add newName to the ui:			
			wchar_t fieldContent[1 + (stringlength * 2)];
			GetDlgItemTextW(IDC_AUTOSAVE_TRACK_FILTER, (LPTSTR)fieldContent, stringlength);

			wchar_t newEntry[stringlength];
			size_t outSize;
			mbstowcs_s(&outSize, newEntry, stringlength, newName.c_str(), stringlength - 1);

			if (fieldContent[0] != L"\0"[0]) {
				wcscat_s(fieldContent, L",");
				//console::formatter() << "fc was determined to not be empty";
			}
			wcscat_s(fieldContent, newEntry);
			
			SetDlgItemText(IDC_AUTOSAVE_TRACK_FILTER, fieldContent);

			return; 
		}

	} else {
		OnChanged();
	}
}

t_uint32 CBookmarkPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CBookmarkPreferences::reset() {
	defToUi(eat_format);
	defToUi(eat_as_newTrackPlaylists);

	defToUi(bab_as_exit);
	defToUi(bab_as_newTrack);
	defToUi(bab_as_newTrackFilter);

	defToUi(bab_verbose);

	OnChanged();
}

void CBookmarkPreferences::apply() {
	uiToCfg(eat_format);
	uiToCfg(eat_as_newTrackPlaylists);

	//Read the checkboxes:
	uiToCfg(bab_as_exit);
	uiToCfg(bab_as_newTrack);
	uiToCfg(bab_as_newTrackFilter);
	uiToCfg(bab_verbose);

	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}

bool CBookmarkPreferences::HasChanged() {
	//returns whether our dialog content is different from the current configuration (whether the apply button should be enabled or not)
	bool result = false;
	result |= isUiChanged(eat_format);
	result |= isUiChanged(eat_as_newTrackPlaylists);

	result |= isUiChanged(bab_as_exit);
	result |= isUiChanged(bab_as_newTrack);
	result |= isUiChanged(bab_as_newTrackFilter);

	result |= isUiChanged(bab_verbose);

	return result;
}
void CBookmarkPreferences::OnChanged() {
	//console::formatter() << "GetCheck currently returns " << ((CCheckBox)GetDlgItem(IDC_AUTOSAVE_EXIT)).GetCheck();

	//Generate the format string preview:
	//1. Obtain current field value
	TCHAR fieldContent[stringlength];
	GetDlgItemTextW(IDC_TITLEFORMAT, (LPTSTR)fieldContent, stringlength);

	char convertedContent[stringlength];
	size_t outSize;
	wcstombs_s(&outSize, convertedContent, fieldContent, stringlength - 1);

	//2. Apply to current song
	pfc::string_formatter songDesc;
	titleformat_object::ptr desc_format;
	static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(desc_format, convertedContent);
	if (!m_playback_control->playback_format_title(NULL, songDesc, desc_format, NULL, playback_control::display_level_all)) {
		songDesc << "Could not generate Description.";
	}
	//3. Output to Textfield
	wchar_t wideString[stringlength];
	mbstowcs_s(&outSize, wideString, stringlength, songDesc.c_str(), stringlength - 1);
	SetDlgItemTextW(IDC_PREVIEW, wideString);

	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CBookmarkPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() { return "Basic Bookmark"; }
	GUID get_guid() {
		// This is our GUID. Replace with your own when reusing the code.
		static const GUID guid = { 0x44604655, 0x45c, 0x4dc5, { 0xbf, 0x2e, 0xe7, 0xf4, 0x48, 0x82, 0x5, 0x30 } };
		return guid;
	}
	GUID get_parent_guid() { return guid_tools; }
};

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;