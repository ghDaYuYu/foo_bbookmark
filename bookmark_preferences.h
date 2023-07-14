#pragma once
extern cfg_string cfg_desc_format;
extern cfg_string cfg_autosave_newtrack_playlists;

extern cfg_bool cfg_autosave_newtrack;
extern cfg_bool cfg_autosave_filter_newtrack;
extern cfg_bool cfg_autosave_on_quit;

extern cfg_bool cfg_verbose;

extern cfg_int cfg_queue_flag;
extern cfg_int cfg_status_flag;
extern cfg_int cfg_misc_flag;


#define QUEUE_RESTORE_TO_FLAG                 1 << 0


#define STATUS_PAUSED_FLAG                    1 << 0


#define MISC_CUST_FLAG                        1 << 0


inline bool is_cfg_Bookmarking() { return !(cfg_status_flag.get_value() & STATUS_PAUSED_FLAG); }
inline bool is_cfg_Queuing() { return cfg_queue_flag.get_value() & QUEUE_RESTORE_TO_FLAG; }