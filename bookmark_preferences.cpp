#include "stdafx.h"
#include "resource.h"

#include <vector>
#include <list>
#include <sstream>

#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>
// CCheckBox
#include <libPPUI/wtl-pp.h>

#include "header_static.h"
#include "bookmark_preferences.h"

#include "bookmark_core.h"

static const int stringlength = 256;

// preference page
static const GUID guid_bookmark_pref_page = { 0x49e82acf, 0x4954, 0x4274, { 0x80, 0xe8, 0xff, 0x74, 0xf3, 0x71, 0x1e, 0x5f } };

static const GUID guid_cfg_desc_format = { 0xa13f4068, 0xa177, 0x4cc0, { 0x9b, 0x5f, 0x4c, 0xe4, 0x85, 0x58, 0xba, 0xfc } };
static const GUID guid_cfg_autosave_newtrack_playlists = { 0x6c5226bc, 0x92a8, 0x4ae8, { 0x85, 0xde, 0xb, 0x58, 0xe3, 0x2b, 0x6e, 0x70 } };

static const GUID guid_cfg_autosave_on_quit = { 0xbce06bc, 0x1d6a, 0x4bc2, { 0xbd, 0xe1, 0x64, 0x91, 0x31, 0x9f, 0xb6, 0xcf } };
static const GUID guid_cfg_autosave_newtrack = { 0x6fc02f38, 0xfd74, 0x4352, { 0xab, 0x8f, 0xac, 0xdd, 0x10, 0x12, 0xb, 0x8f } };
static const GUID guid_cfg_autosave_filter_newtrack = { 0x75728bc2, 0x6955, 0x4ea6, { 0x95, 0xe5, 0xf3, 0xb2, 0xfc, 0xef, 0x3c, 0x8c } };

static const GUID guid_cfg_verbose = { 0x354baaa6, 0x7bbb, 0x40df, { 0xbf, 0xb8, 0x8b, 0x76, 0xc, 0xbd, 0x9d, 0xd0 } };

// {E0B79D39-269C-49ED-8892-ED46DD5F3445}
static const GUID guid_cfg_queue_flag = { 0xe0b79d39, 0x269c, 0x49ed, { 0x88, 0x92, 0xed, 0x46, 0xdd, 0x5f, 0x34, 0x45 } };

// {3B8608CE-F964-463D-9011-41999D4E0DD9}
static const GUID guid_cfg_status_flag = { 0x3b8608ce, 0xf964, 0x463d, { 0x90, 0x11, 0x41, 0x99, 0x9d, 0x4e, 0xd, 0xd9 } };

// {452AC946-F849-4C79-9868-01C60F0421E6}
static const GUID guid_cfg_misc_flag = { 0x452ac946, 0xf849, 0x4c79, { 0x98, 0x68, 0x1, 0xc6, 0xf, 0x4, 0x21, 0xe6 } };

// defaults

static const pfc::string8 default_cfg_bookmark_desc_format = "%title%";
static const pfc::string8 default_cfg_autosave_newtrack_playlists = "Podcatcher";

static const bool default_cfg_autosave_newtrack = false;
static const bool default_cfg_autosave_filter_newtrack = true;
static const bool default_cfg_autosave_on_quit = false;

static const bool default_cfg_verbose = false;

static const int default_cfg_queue_flag = 0;
static const int default_cfg_status_flag = 0;
static const int default_cfg_misc_flag = 0;

// cfg_var

cfg_string cfg_desc_format(guid_cfg_desc_format, default_cfg_bookmark_desc_format.c_str());
cfg_string cfg_autosave_newtrack_playlists(guid_cfg_autosave_newtrack_playlists, default_cfg_autosave_newtrack_playlists.c_str());

cfg_bool cfg_autosave_newtrack(guid_cfg_autosave_newtrack, default_cfg_autosave_newtrack);
cfg_bool cfg_autosave_filter_newtrack(guid_cfg_autosave_filter_newtrack, default_cfg_autosave_filter_newtrack);
cfg_bool cfg_autosave_on_quit(guid_cfg_autosave_on_quit, default_cfg_autosave_on_quit);

cfg_bool cfg_verbose(guid_cfg_verbose, default_cfg_verbose);

cfg_int cfg_queue_flag(guid_cfg_queue_flag, default_cfg_queue_flag);
cfg_int cfg_status_flag(guid_cfg_status_flag, default_cfg_status_flag);
cfg_int cfg_misc_flag(guid_cfg_misc_flag, default_cfg_misc_flag);

struct boxAndBool_t {
	int idc;
	cfg_bool* cfg;
	bool def;
};

struct boxAndInt_t {
	int idc;
	cfg_int* cfg;
	int def;
};

struct ectrlAndString_t {
	int idc;
	cfg_string* cfg;
	pfc::string8 def;
};


class CBookmarkPreferences : public CDialogImpl<CBookmarkPreferences>, public preferences_page_instance {

public:

	CBookmarkPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {
		//..
	}

	~CBookmarkPreferences() { 
		g_wnd_bookmark_pref = NULL;
		m_staticPrefHeader.Detach();
	}

	enum { IDD = IDD_BOOKMARK_PREFERENCES };

	t_uint32 get_state() override;
	void apply() override;
	void reset() override;

	BEGIN_MSG_MAP_EX(CBookmarkPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_CODE_HANDLER_EX(EN_CHANGE, OnEditChange)
		COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnCheckChange)
		MESSAGE_HANDLER_SIMPLE(UMSG_NEW_TRACK, OnNewTrackMessage)
		MESSAGE_HANDLER_SIMPLE(UMSG_PAUSED, OnPaused)
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT uNotifyCode, int nId, CWindow wndCtl);
	void OnCheckChange(UINT uNotifyCode, int nId, CWindow wndCtl);

	bool HasChanged();
	void OnChanged();

	LRESULT OnNewTrackMessage() { OnChanged(); return 0; }

	LRESULT OnPaused() { cfgToUi(bai_status_flag); HasChanged(); return 0; }

	// boxAndBool_t

	void cfgToUi(boxAndBool_t bab) {
		CCheckBox cb(GetDlgItem(bab.idc));
		cb.SetCheck(*(bab.cfg));
	}

	void uiToCfg(boxAndBool_t & bab) {
		CCheckBox cb(GetDlgItem(bab.idc));
		bab.cfg->set((bool)cb.GetCheck());
	}

	void defToUi(boxAndBool_t bab) {
		CCheckBox cb(GetDlgItem(bab.idc));
		cb.SetCheck(bab.def);
	}

	bool isUiChanged(boxAndBool_t bab) {
		CCheckBox cb(GetDlgItem(bab.idc));
		return *(bab.cfg) != (bool)cb.GetCheck();
	}

	// boxAndInt_t

	void cfgToUi(boxAndInt_t bai) {
		CCheckBox cb(GetDlgItem(bai.idc));
		cb.SetCheck((bool) (bai.cfg->get_value()));
	}

	void uiToCfg(boxAndInt_t & bai) {
		CCheckBox cb(GetDlgItem(bai.idc));
		bai.cfg->set(cb.GetCheck());
	}

	void defToUi(boxAndInt_t bai) {
		CCheckBox cb(GetDlgItem(bai.idc));
		cb.SetCheck(bai.def);
	}

	bool isUiChanged(boxAndInt_t bai) {
		CCheckBox cb(GetDlgItem(bai.idc));
		return *(bai.cfg) != cb.GetCheck();
	}

	// ectrlAndString_t

	void cfgToUi(ectrlAndString_t eat) {

		uSetDlgItemText(m_hWnd, eat.idc, eat.cfg->get_value().c_str());
	}

	void uiToCfg(ectrlAndString_t & eat) {

		pfc::string8 buffer = uGetDlgItemText(m_hWnd, eat.idc);
		eat.cfg->set(buffer.c_str());
	}

	void defToUi(ectrlAndString_t eat) {

		uSetDlgItemText(m_hWnd, eat.idc, eat.def.c_str());
	}

	bool isUiChanged(ectrlAndString_t eat) {

		pfc::string8 buffer = uGetDlgItemText(m_hWnd, eat.idc);
		return !buffer.equals(eat.cfg->get_value());
	}


public:

	std::vector<pfc::string8> m_currentPlNames;

private:

	HeaderStatic m_staticPrefHeader;
	fb2k::CDarkModeHooks m_dark;

	static_api_ptr_t<playback_control> m_playback_control;
	const preferences_page_callback::ptr m_callback;

	//TODO: group all these, then use for loops
	ectrlAndString_t eat_format = { IDC_TITLEFORMAT, &cfg_desc_format, default_cfg_bookmark_desc_format };
	ectrlAndString_t eat_as_newtrack_playlists = { IDC_AUTOSAVE_TRACK_FILTER, &cfg_autosave_newtrack_playlists, default_cfg_autosave_newtrack_playlists };

	boxAndBool_t bab_as_newtrack = { IDC_AUTOSAVE_TRACK, &cfg_autosave_newtrack, default_cfg_autosave_newtrack };
	boxAndBool_t bab_as_filter_newtrack = { IDC_AUTOSAVE_TRACK_FILTER_CHECK, &cfg_autosave_filter_newtrack, default_cfg_autosave_filter_newtrack };
	boxAndBool_t bab_as_exit = { IDC_AUTOSAVE_EXIT, &cfg_autosave_on_quit, default_cfg_autosave_on_quit };

	boxAndBool_t bab_verbose = { IDC_VERBOSE, &cfg_verbose, default_cfg_verbose };

	boxAndInt_t bai_queue_flag = { IDC_QUEUE_FLAG, &cfg_queue_flag, default_cfg_queue_flag };
	boxAndInt_t bai_status_flag = { IDC_STATUS_FLAG, &cfg_status_flag, default_cfg_status_flag };
	boxAndInt_t bai_misc_flag = { IDC_MISC_FLAG, &cfg_misc_flag, default_cfg_misc_flag };

};

BOOL CBookmarkPreferences::OnInitDialog(CWindow wndCtl, LPARAM) {

	g_wnd_bookmark_pref = m_hWnd;

	cfgToUi(eat_format);
	cfgToUi(eat_as_newtrack_playlists);

	cfgToUi(bab_as_exit);
	cfgToUi(bab_as_newtrack);
	cfgToUi(bab_as_filter_newtrack);

	cfgToUi(bab_verbose);

	cfgToUi(bai_queue_flag);
	cfgToUi(bai_status_flag);
	cfgToUi(bai_misc_flag);

	//static header

	HWND wndStaticHeader = uGetDlgItem(IDC_STATIC_PREF_HEADER);
	m_staticPrefHeader.SubclassWindow(wndStaticHeader);
	m_staticPrefHeader.PaintGradientHeader();

	//fill playlist combo

	m_currentPlNames.clear();
	size_t plCount = playlist_manager::get()->get_playlist_count();
	CComboBox comboBox = GetDlgItem(IDC_CMB_PLAYLISTS);
	
	for (size_t i = 0; i < plCount; i++) {

		pfc::string8 plName;
		playlist_manager::get()->playlist_get_name(i, plName);
		FB2K_console_print("found playlist called ", plName.c_str());
		m_currentPlNames.emplace_back(plName);

		size_t outSize;
		wchar_t wideString[stringlength];
		mbstowcs_s(&outSize, wideString, stringlength, plName.c_str(), stringlength - 1);

		SendMessage(comboBox, CB_ADDSTRING, 0, (LPARAM)wideString);
	}
	SendMessage(comboBox, CB_SETCURSEL, 0, 0);

	comboBox.SetTopIndex(0);

	//dark mode
	m_dark.AddDialogWithControls(*this);

	return FALSE;
}

void CBookmarkPreferences::OnEditChange(UINT uNotifyCode, int nId, CWindow wndCtl) {

	OnChanged();
}

void CBookmarkPreferences::OnCheckChange(UINT uNotifyCode, int nId, CWindow wndCtl) {

	if (nId == IDC_BTN_ADD_EXISTING_PLAYLIST) {

		//Todo: add currently selected playlist to the filter edit control
		CComboBox comboBox = GetDlgItem(IDC_CMB_PLAYLISTS);
		int selected = comboBox.GetCurSel();

		if (selected >= 0 && selected < m_currentPlNames.size()) {
			pfc::string8 newName;
			newName += m_currentPlNames[selected];
			//replace all commas with dots (because of the comma-seperated list)
			newName.replace_char(',', '.');

			//check if name already exists
			std::stringstream ss(cfg_autosave_newtrack_playlists.get_value().c_str());
			std::string token;
			while (std::getline(ss, token, ',')) {
				if (strcmp(token.c_str(), newName.c_str()) == 0) {
					//skip
					return;
				}
			}

			FB2K_console_print("adding to auto-bookmarking playlists: ", newName);

			//Add newName to the ui:			
			wchar_t fieldContent[1 + (stringlength * 2)];
			GetDlgItemTextW(IDC_AUTOSAVE_TRACK_FILTER, (LPTSTR)fieldContent, stringlength);

			wchar_t newEntry[stringlength];
			size_t outSize;
			mbstowcs_s(&outSize, newEntry, stringlength, newName.c_str(), stringlength - 1);

			if (fieldContent[0] != L"\0"[0]) {
				wcscat_s(fieldContent, L",");
				//FB2K_console_print("fc was determined to not be empty");
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

	t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
	
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}

void CBookmarkPreferences::reset() {

	defToUi(eat_format);
	defToUi(eat_as_newtrack_playlists);

	defToUi(bab_as_exit);
	defToUi(bab_as_newtrack);
	defToUi(bab_as_filter_newtrack);

	defToUi(bab_verbose);

	defToUi(bai_queue_flag);
	defToUi(bai_status_flag);
	defToUi(bai_misc_flag);

	OnChanged();
}

void CBookmarkPreferences::apply() {

	uiToCfg(eat_format);
	uiToCfg(eat_as_newtrack_playlists);

	uiToCfg(bab_as_exit);
	uiToCfg(bab_as_newtrack);

	//refresh dummy
	if (bab_as_newtrack.cfg->get()) {
		g_bmAuto.updateDummy();
	}

	uiToCfg(bab_as_filter_newtrack);
	uiToCfg(bab_verbose);

	uiToCfg(bai_queue_flag);
	uiToCfg(bai_status_flag);
	uiToCfg(bai_misc_flag);

	OnChanged();
}

bool CBookmarkPreferences::HasChanged() {

	bool result = false;
	result |= isUiChanged(eat_format);
	result |= isUiChanged(eat_as_newtrack_playlists);

	result |= isUiChanged(bab_as_exit);
	result |= isUiChanged(bab_as_newtrack);
	result |= isUiChanged(bab_as_filter_newtrack);

	result |= isUiChanged(bab_verbose);

	result |= isUiChanged(bai_queue_flag);

	result |= isUiChanged(bai_status_flag);
	result |= isUiChanged(bai_misc_flag);

	return result;
}

void CBookmarkPreferences::OnChanged() {

	titleformat_object::ptr p_script;
	pfc::string8 titleformat = uGetDlgItemText(m_hWnd, IDC_TITLEFORMAT);
	static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(p_script, titleformat);
	
	pfc::string_formatter songDesc;
	if (!m_playback_control->playback_format_title(NULL, songDesc, p_script, NULL, playback_control::display_level_all)) {
		songDesc << "Resume playback to generate track description.";
	}

	const pfc::stringcvt::string_os_from_utf8 os_tag_name(songDesc);

	SetDlgItemTextW(IDC_PREVIEW, os_tag_name);

	//enable/disable the apply button
	m_callback->on_state_changed();
}

class preferences_page_myimpl : public preferences_page_impl<CBookmarkPreferences> {

public:

	const char * get_name() override { return COMPONENT_NAME_HC; }
	GUID get_guid() override { return guid_bookmark_pref_page; }
	GUID get_parent_guid() override { return guid_tools; }
};

static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;