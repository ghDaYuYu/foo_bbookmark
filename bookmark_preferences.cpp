#include "stdafx.h"
#include "resource.h"

#include <vector>
#include <list>
#include <sstream>
#include "atlframe.h"
#include "atlwin.h"

#include <helpers/atl-misc.h>
#include <helpers/DarkMode.h>
// CCheckBox
#include <libPPUI/wtl-pp.h>
#include <libPPUI/CDialogResizeHelper.h>

#include "header_static.h"

#include "bookmark_core.h"
#include "bookmark_automatic.h"

static const int stringlength = 256;

// preference page
static const GUID guid_bookmark_pref_page = { 0x49e82acf, 0x4954, 0x4274, { 0x80, 0xe8, 0xff, 0x74, 0xf3, 0x71, 0x1e, 0x5f } };
GUID  g_get_prefs_guid() { return guid_bookmark_pref_page; }

static const GUID guid_cfg_desc_format = { 0xa13f4068, 0xa177, 0x4cc0, { 0x9b, 0x5f, 0x4c, 0xe4, 0x85, 0x58, 0xba, 0xfc } };
static const GUID guid_cfg_date_format = { 0x25c3c9bd, 0x80b3, 0x4926, { 0xb0, 0x6, 0xed, 0x7b, 0xb9, 0x9c, 0x1f, 0x1 } };
static const GUID guid_cfg_display_ms = { 0xd7983d68, 0x7073, 0x4ae6, { 0x95, 0x1f, 0xc3, 0x46, 0xcd, 0xb, 0x3e, 0xf6 } };
static const GUID guid_cfg_autosave_newtrack_playlists = { 0x6c5226bc, 0x92a8, 0x4ae8, { 0x85, 0xde, 0xb, 0x58, 0xe3, 0x2b, 0x6e, 0x70 } };
static const GUID guid_cfg_autosave_on_quit = { 0xbce06bc, 0x1d6a, 0x4bc2, { 0xbd, 0xe1, 0x64, 0x91, 0x31, 0x9f, 0xb6, 0xcf } };
static const GUID guid_cfg_autosave_newtrack = { 0x6fc02f38, 0xfd74, 0x4352, { 0xab, 0x8f, 0xac, 0xdd, 0x10, 0x12, 0xb, 0x8f } };
static const GUID guid_cfg_autosave_focus_newtrack = { 0x612365c1, 0x8b58, 0x434f, { 0xb8, 0xb1, 0x6f, 0x72, 0x2a, 0x85, 0x21, 0xf6 } };
static const GUID guid_cfg_autosave_radio_newtrack = { 0x87cc64d0, 0x360e, 0x4c21, { 0xb9, 0x8, 0x2d, 0x94, 0x65, 0x67, 0x4a, 0xfc } };
static const GUID guid_cfg_autosave_radio_comment = { 0x9fa5bf2b, 0xb0ea, 0x416e, { 0xb7, 0x6, 0x35, 0x3c, 0x76, 0xbb, 0x99, 0xd3 } };
static const GUID guid_cfg_autosave_filter_newtrack = { 0x75728bc2, 0x6955, 0x4ea6, { 0x95, 0xe5, 0xf3, 0xb2, 0xfc, 0xef, 0x3c, 0x8c } };

static const GUID guid_cfg_verbose = { 0x354baaa6, 0x7bbb, 0x40df, { 0xbf, 0xb8, 0x8b, 0x76, 0xc, 0xbd, 0x9d, 0xd0 } };
static const GUID guid_cfg_monitor = { 0xd36b1b6d, 0x4a55, 0x48c1, { 0xab, 0x46, 0x69, 0x14, 0xb5, 0x5, 0x2f, 0x7f } };

// {E0B79D39-269C-49ED-8892-ED46DD5F3445}
static const GUID guid_cfg_queue_flag = { 0xe0b79d39, 0x269c, 0x49ed, { 0x88, 0x92, 0xed, 0x46, 0xdd, 0x5f, 0x34, 0x45 } };

// {3B8608CE-F964-463D-9011-41999D4E0DD9}
static const GUID guid_cfg_status_flag = { 0x3b8608ce, 0xf964, 0x463d, { 0x90, 0x11, 0x41, 0x99, 0x9d, 0x4e, 0xd, 0xd9 } };

// {452AC946-F849-4C79-9868-01C60F0421E6}
static const GUID guid_cfg_misc_flag = { 0x452ac946, 0xf849, 0x4c79, { 0x98, 0x68, 0x1, 0xc6, 0xf, 0x4, 0x21, 0xe6 } };

// defaults

static const pfc::string8 default_cfg_bookmark_desc_format = "%title% - $if2(%album% - ,- )%artist%";
static const pfc::string8 default_cfg_date_format = "%a %b %d %H:%M:%S %Y";
static const bool default_cfg_display_ms = false;
static const pfc::string8 default_cfg_autosave_newtrack_playlists = "Podcatcher";

static const bool default_cfg_autosave_newtrack = false;
static const bool default_cfg_autosave_focus_newtrack = true;
static const bool default_cfg_autosave_radio_newtrack = false;
static const bool default_cfg_autosave_radio_comment = false;
static const bool default_cfg_autosave_filter_newtrack = false;
static const bool default_cfg_autosave_on_quit = false;

static const bool default_cfg_verbose = false;
static const bool default_cfg_monitor = true;

static const int default_cfg_queue_flag = 0;
static const int default_cfg_status_flag = 0;
static const int default_cfg_misc_flag = 0;

// cfg_var

cfg_string cfg_desc_format(guid_cfg_desc_format, default_cfg_bookmark_desc_format.c_str());
cfg_string cfg_date_format(guid_cfg_date_format, default_cfg_date_format.c_str());
cfg_bool cfg_display_ms(guid_cfg_display_ms, default_cfg_display_ms);
cfg_string cfg_autosave_newtrack_playlists(guid_cfg_autosave_newtrack_playlists, default_cfg_autosave_newtrack_playlists.c_str());

cfg_bool cfg_autosave_newtrack(guid_cfg_autosave_newtrack, default_cfg_autosave_newtrack);
cfg_bool cfg_autosave_focus_newtrack(guid_cfg_autosave_focus_newtrack, default_cfg_autosave_focus_newtrack);
cfg_bool cfg_autosave_radio_newtrack(guid_cfg_autosave_radio_newtrack, default_cfg_autosave_radio_newtrack);
cfg_bool cfg_autosave_radio_comment(guid_cfg_autosave_radio_comment, default_cfg_autosave_radio_comment);
cfg_bool cfg_autosave_filter_newtrack(guid_cfg_autosave_filter_newtrack, default_cfg_autosave_filter_newtrack);
cfg_bool cfg_autosave_on_quit(guid_cfg_autosave_on_quit, default_cfg_autosave_on_quit);

cfg_bool cfg_verbose(guid_cfg_verbose, default_cfg_verbose);
cfg_bool cfg_monitor(guid_cfg_monitor, default_cfg_monitor);

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

//snapLeft, snapTop, snapRight, snapBottom
const CDialogResizeHelper::Param resize_params[] = {
	{IDC_STATIC_PREF_HEADER, 0,0,1,0},
	{IDC_TITLEFORMAT, 0,0,1,0},
	{IDC_PREVIEW, 0,0,1,0},
	{IDC_MONITOR, 0,0,1,0},
	{IDC_QUEUE_FLAG, 0,0,1,0},
	{IDC_AUTOSAVE_RADIO_TRACK, 1,0,1,0},
	{IDC_AUTOSAVE_RADIO_COMMENT_ST, 1,0,1,0},
	{IDC_DISPLAY_MS, 1,0,1,0},
	{IDC_STATIC_DISPLAY_MS, 1,0,1,0},
};

class CBookmarkPreferences : public CDialogImpl<CBookmarkPreferences>,
	public preferences_page_instance {

public:

	CBookmarkPreferences(preferences_page_callback::ptr callback) : m_callback(callback),
			m_resize_helper(resize_params) {
		//..
	}

	~CBookmarkPreferences() { 
		glb::g_wnd_bookmark_pref = NULL;
		m_staticPrefHeader.Detach();
	}

	enum { IDD = IDD_BOOKMARK_PREFERENCES };

	t_uint32 get_state() override;
	void apply() override;
	void reset() override;

	BEGIN_MSG_MAP_EX(CBookmarkPreferences)
		CHAIN_MSG_MAP_MEMBER(m_resize_helper)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_CODE_HANDLER_EX(EN_CHANGE, OnEditChange)
		COMMAND_CODE_HANDLER_EX(CBN_SELCHANGE, OnComboChange)
		COMMAND_CODE_HANDLER_EX(BN_CLICKED, OnCheckChange)
		MESSAGE_HANDLER_SIMPLE(glb::UMSG_NEW_TRACK, OnNewTrackMessage)
		MESSAGE_HANDLER_SIMPLE(glb::UMSG_PAUSED, OnPaused)
	END_MSG_MAP()


private:

	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT uNotifyCode, int nId, CWindow wndCtl);
	void OnComboChange(UINT uNotifyCode, int nId, CWindow wndCtl);
	void OnCheckChange(UINT uNotifyCode, int nId, CWindow wndCtl);

	bool HasChanged();
	void OnChanged();

	LRESULT OnNewTrackMessage() { OnChanged(); return 0; }

	LRESULT OnPaused() { cfgToUi(bai_status_flag); HasChanged(); return 0; }

	// boxAndBool_t

	void cfgToUi(boxAndBool_t bab) {
		CCheckBox cb(GetDlgItem(bab.idc));
		cb.SetCheck(bab.cfg->get());
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
		return bab.cfg->get() != (bool)cb.GetCheck();
	}

	// boxAndInt_t

	void cfgToUi(boxAndInt_t bai) {
		CCheckBox cb(GetDlgItem(bai.idc));
		cb.SetCheck((bool) (bai.cfg->get_value()));
	}

	void cfgToUi(boxAndInt_t bai, int flag, int idc) {
		CCheckBox cb(GetDlgItem(idc));
		cb.SetCheck(bai.cfg->get_value() & flag);
	}

	void uiToCfg(boxAndInt_t & bai) {
		CCheckBox cb(GetDlgItem(bai.idc));
		bai.cfg->set(cb.GetCheck());
	}

	void uiToCfg(boxAndInt_t& bai, int ui_fval) {
		bai.cfg->set(ui_fval);
	}

	void defToUi(boxAndInt_t bai) {
		CCheckBox cb(GetDlgItem(bai.idc));
		cb.SetCheck(bai.def);
	}

	void defToUi(boxAndInt_t bai, int flag, int idc) {
		CCheckBox cb(GetDlgItem(idc));
		cb.SetCheck(bai.def & flag);
	}

	bool isUiChanged(boxAndInt_t bai) {
		CCheckBox cb(GetDlgItem(bai.idc));
		return bai.cfg->get_value() != (int)cb.GetCheck();
	}

	bool isUiChanged(boxAndInt_t bai, int ui_fval) {
		return bai.cfg->get_value() != ui_fval;
	}

	// ectrlAndString_t

	void cfgToUi(ectrlAndString_t eat) {
		if (eat.idc == IDC_CMB_DATEFORMAT) {
			uSetDlgItemText(m_hWnd, eat.idc, eat.cfg->get_value().c_str());
		}
		else {
			uSetDlgItemText(m_hWnd, eat.idc, eat.cfg->get_value().c_str());
		}
	}

	void uiToCfg(ectrlAndString_t & eat) {
		pfc::string8 buffer;
		if (eat.idc == IDC_CMB_DATEFORMAT) {
			buffer = uGetDlgItemText(m_hWnd, eat.idc);
		}
		else {
			buffer = uGetDlgItemText(m_hWnd, eat.idc);
		}
		eat.cfg->set(buffer.c_str());
	}

	void defToUi(ectrlAndString_t eat) {
		if (eat.idc == IDC_CMB_DATEFORMAT) {
			uSetDlgItemText(m_hWnd, eat.idc, eat.def.c_str());
		}
		else {
			uSetDlgItemText(m_hWnd, eat.idc, eat.def.c_str());
		}
	}

	bool isUiChanged(ectrlAndString_t eat) {
		pfc::string8 buffer;
		if (eat.idc == IDC_CMB_DATEFORMAT) {
			buffer = uGetDlgItemText(m_hWnd, eat.idc);
		}
		else {
			buffer = uGetDlgItemText(m_hWnd, eat.idc);
		}
		return !buffer.equals(eat.cfg->get_value());
	}

public:

	std::vector<pfc::string8> m_currentPlNames;

private:

	CDialogResizeHelper m_resize_helper;
	HeaderStatic m_staticPrefHeader;
	fb2k::CDarkModeHooks m_dark;

	static_api_ptr_t<playback_control> m_playback_control;
	const preferences_page_callback::ptr m_callback;

	//TODO: group all these, then use for loops
	ectrlAndString_t eat_format = { IDC_TITLEFORMAT, &cfg_desc_format, default_cfg_bookmark_desc_format };
	ectrlAndString_t eat_date = { IDC_CMB_DATEFORMAT, &cfg_date_format, default_cfg_date_format };
	boxAndBool_t bab_display_ms = { IDC_DISPLAY_MS, &cfg_display_ms, default_cfg_display_ms };
	ectrlAndString_t eat_as_newtrack_playlists = { IDC_AUTOSAVE_TRACK_FILTER, &cfg_autosave_newtrack_playlists, default_cfg_autosave_newtrack_playlists };

	boxAndBool_t bab_as_newtrack = { IDC_AUTOSAVE_TRACK, &cfg_autosave_newtrack, default_cfg_autosave_newtrack };
	boxAndBool_t bab_as_focus_newtrack = { IDC_AUTOSAVE_FOCUS_TRACK, &cfg_autosave_focus_newtrack, default_cfg_autosave_focus_newtrack };
	boxAndBool_t bab_as_radio_newtrack = { IDC_AUTOSAVE_RADIO_TRACK, &cfg_autosave_radio_newtrack, default_cfg_autosave_radio_newtrack };
	boxAndBool_t bab_as_radio_comment = { IDC_AUTOSAVE_RADIO_COMMENT_ST, &cfg_autosave_radio_comment, default_cfg_autosave_radio_comment };
	boxAndBool_t bab_as_filter_newtrack = { IDC_AUTOSAVE_TRACK_FILTER_CHECK, &cfg_autosave_filter_newtrack, default_cfg_autosave_filter_newtrack };
	boxAndBool_t bab_as_exit = { IDC_AUTOSAVE_EXIT, &cfg_autosave_on_quit, default_cfg_autosave_on_quit };

	boxAndBool_t bab_verbose = { IDC_VERBOSE, &cfg_verbose, default_cfg_verbose };
	boxAndBool_t bab_monitor = { IDC_MONITOR, &cfg_monitor, default_cfg_monitor };

	boxAndInt_t bai_queue_flag = { IDC_QUEUE_FLAG, &cfg_queue_flag, default_cfg_queue_flag };
	boxAndInt_t bai_status_flag = { IDC_STATUS_FLAG, &cfg_status_flag, default_cfg_status_flag };
	boxAndInt_t bai_misc_flag = { IDC_MISC_FLAG, &cfg_misc_flag, default_cfg_misc_flag };

};

void ConvertString8(const pfc::string8 orig, wchar_t* out, size_t max) {
	pfc::stringcvt::convert_utf8_to_wide(out, max, orig.get_ptr(), orig.length());
}

void InitDateCombo(HWND hwndParent, UINT idc_date, pfc::string8 strval) {

	std::vector<std::string> vfd = {
		"%a %b %d %H:%M:%S %Y",
		"%y-%m-%d %H:%M:%S %a",
		"%y-%m-%d %H:%M %a",
		"%y-%m-%d %H:%M"
	};

	CComboBox cmb = GetDlgItem(hwndParent, idc_date);

	for (auto& w : vfd) {
		WCHAR wstr[DATE_BUFFER_SIZE];
		ConvertString8(w.c_str(), wstr, DATE_BUFFER_SIZE - 1);
		cmb.SetItemData(cmb.AddString(wstr), cmb.GetCount());
	}

	auto cursel = std::find(vfd.begin(), vfd.end(), strval.c_str());
	auto curndx = std::distance(vfd.begin(), cursel);

	for (int i = 0; i < cmb.GetCount(); ++i) {
		WCHAR wstr[DATE_BUFFER_SIZE];
		ConvertString8(strval.c_str(), wstr, DATE_BUFFER_SIZE - 1);
		auto dbg = cmb.GetCurSel();
		if (cmb.GetItemData(i) == curndx)
		{
			cmb.SetCurSel(i);
			break;
		}
	}
}

BOOL CBookmarkPreferences::OnInitDialog(CWindow, LPARAM) {

	glb::g_wnd_bookmark_pref = m_hWnd;

	InitDateCombo(m_hWnd, eat_date.idc, eat_date.cfg->get_value());

	cfgToUi(eat_format);
	cfgToUi(eat_date);
	cfgToUi(bab_display_ms);
	cfgToUi(eat_as_newtrack_playlists);

	cfgToUi(bab_as_exit);
	cfgToUi(bab_as_newtrack);
	cfgToUi(bab_as_focus_newtrack);
	cfgToUi(bab_as_radio_newtrack);
	cfgToUi(bab_as_radio_comment);
	cfgToUi(bab_as_filter_newtrack);

	cfgToUi(bab_verbose);
	cfgToUi(bab_monitor);

	cfgToUi(bai_queue_flag, QUEUE_RESTORE_TO_FLAG, /*IDC_QUEUE_FLAG*/bai_queue_flag.idc);
	cfgToUi(bai_queue_flag, QUEUE_FLUSH_FLAG, IDC_QUEUE_FLUSH_FLAG);

	cfgToUi(bai_status_flag);
	cfgToUi(bai_misc_flag);

	//static header

	HWND wndStaticHeader = uGetDlgItem(IDC_STATIC_PREF_HEADER);
	m_staticPrefHeader.SubclassWindow(wndStaticHeader);
	m_staticPrefHeader.PaintHeader();

	//fill playlist combo

	m_currentPlNames.clear();
	size_t plCount = playlist_manager::get()->get_playlist_count();
	CComboBox comboBox = GetDlgItem(IDC_CMB_PLAYLISTS);
	
	for (size_t i = 0; i < plCount; i++) {

		pfc::string8 plName;
		playlist_manager::get()->playlist_get_name(i, plName);

		m_currentPlNames.emplace_back(plName);

		WCHAR wstr[1024];
		ConvertString8(plName, wstr, 1024 - 1);

		uSendMessage(comboBox, CB_ADDSTRING, 0, (LPARAM)wstr);

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

void CBookmarkPreferences::OnComboChange(UINT uNotifyCode, int nId, CWindow wndCtl) {
	if (nId != IDC_CMB_DATEFORMAT) {
		//nothing to do
		return;
	}
	pfc::string8 strFormat = uGetDlgItemText(m_hWnd, nId);

	auto t = std::time(nullptr);
	auto tm = *std::localtime(&t);
	auto sctime = asctime(&tm);

	char buffer[DATE_BUFFER_SIZE];
	std::strftime(buffer, DATE_BUFFER_SIZE, strFormat, &tm);


	wchar_t fieldContent[1 + (stringlength * 2)];

	if (fieldContent[0] != L"\0"[0]) {
		auto dbg = 0;
	}

	WCHAR wstr[stringlength];
	ConvertString8(buffer, wstr, stringlength - 1);

	SetDlgItemTextW(IDC_PREVIEW_DATE_FORMAT, wstr);
	m_callback->on_state_changed();
}

void CBookmarkPreferences::OnCheckChange(UINT uNotifyCode, int nId, CWindow wndCtl) {

	if (nId == IDC_BTN_ADD_EXISTING_PLAYLIST) {

		//Todo: add currently selected playlist to the filter edit control
		CComboBox comboBox = GetDlgItem(IDC_CMB_PLAYLISTS);
		int selected = comboBox.GetCurSel();

		if (selected >= 0 && static_cast<size_t>(selected) < m_currentPlNames.size()) {
			pfc::string8 newName;
			newName += m_currentPlNames[selected];

			//replace all commas with dots (because of the comma-seperated list)
			newName.replace_char(',', '.');

			pfc::string8 curr_filter;
			uGetDlgItemText(m_hWnd, IDC_AUTOSAVE_TRACK_FILTER, curr_filter);
			//check if name already exists
			std::stringstream ss(curr_filter.c_str());
			std::string token;
			while (std::getline(ss, token, ',')) {
				if (!stricmp_utf8(token.c_str(), newName.c_str())) {
					//skip
					return;
				}
			}

			FB2K_console_print_v("Adding to auto-bookmarking playlists: ", newName);

			//Add newName to the ui:
			wchar_t fieldContent[1 + (stringlength * 2)];
			GetDlgItemTextW(IDC_AUTOSAVE_TRACK_FILTER, (LPTSTR)fieldContent, stringlength);

			if (fieldContent[0] != L"\0"[0]) {
				wcscat_s(fieldContent, L",");
			}
			
			WCHAR wstr[1024];
			ConvertString8(newName, wstr, 1024 - 1);

			wcscat_s(fieldContent, wstr);

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
	defToUi(eat_date);
	defToUi(bab_display_ms);
	defToUi(eat_as_newtrack_playlists);

	defToUi(bab_as_exit);
	defToUi(bab_as_newtrack);
	defToUi(bab_as_focus_newtrack);
	defToUi(bab_as_radio_newtrack);
	defToUi(bab_as_radio_comment);
	defToUi(bab_as_filter_newtrack);

	defToUi(bab_verbose);
	defToUi(bab_monitor);

	defToUi(bai_queue_flag, QUEUE_RESTORE_TO_FLAG, /*IDC_QUEUE_FLAG*/bai_queue_flag.idc);
	defToUi(bai_queue_flag, QUEUE_FLUSH_FLAG, IDC_QUEUE_FLUSH_FLAG);

	defToUi(bai_status_flag);
	defToUi(bai_misc_flag);

	OnChanged();
}

void CBookmarkPreferences::apply() {

	bool bneedReload = isUiChanged(bab_display_ms);

	uiToCfg(eat_format);
	uiToCfg(eat_date);
	uiToCfg(bab_display_ms);
	uiToCfg(eat_as_newtrack_playlists);

	uiToCfg(bab_as_exit);
	uiToCfg(bab_as_newtrack);
	uiToCfg(bab_as_focus_newtrack);
	uiToCfg(bab_as_radio_newtrack);
	uiToCfg(bab_as_radio_comment);
	uiToCfg(bab_as_filter_newtrack);

	//refresh dummy
	if (bab_as_newtrack.cfg->get()) {
		glb::g_bmAuto.updateDummy();
	}

	uiToCfg(bab_as_filter_newtrack);
	uiToCfg(bab_verbose);
	uiToCfg(bab_monitor);

	int ui_fval = 0;
	ui_fval = IsDlgButtonChecked(bai_queue_flag.idc) ? QUEUE_RESTORE_TO_FLAG : ui_fval;
	ui_fval = IsDlgButtonChecked(IDC_QUEUE_FLUSH_FLAG) ? ui_fval | QUEUE_FLUSH_FLAG : ui_fval;
	uiToCfg(bai_queue_flag, ui_fval);
	uiToCfg(bai_status_flag);
	uiToCfg(bai_misc_flag);


	if (bneedReload) {
		for (auto gui : glb::g_guiLists) {
			gui->ReloadItems(bit_array_true());
		}
	}

	OnChanged();
}

bool CBookmarkPreferences::HasChanged() {

	bool result = false;
	result |= isUiChanged(eat_format);
	result |= isUiChanged(eat_date);
	result |= isUiChanged(bab_display_ms);
	result |= isUiChanged(eat_as_newtrack_playlists);

	result |= isUiChanged(bab_as_exit);
	result |= isUiChanged(bab_as_newtrack);
	result |= isUiChanged(bab_as_focus_newtrack);
	result |= isUiChanged(bab_as_radio_newtrack);
	result |= isUiChanged(bab_as_radio_comment);
	result |= isUiChanged(bab_as_filter_newtrack);

	result |= isUiChanged(bab_verbose);
	result |= isUiChanged(bab_monitor);

	int ui_fval = 0;
	ui_fval = IsDlgButtonChecked(/*IDC_QUEUE_FLAG*/bai_queue_flag.idc) ? QUEUE_RESTORE_TO_FLAG : ui_fval;
	ui_fval = IsDlgButtonChecked(IDC_QUEUE_FLUSH_FLAG) ? ui_fval | QUEUE_FLUSH_FLAG : ui_fval;
	result |= isUiChanged(bai_queue_flag, ui_fval);

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
		songDesc << "(resume playback to generate track description)";
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