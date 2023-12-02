
#ifndef WINVER
#define WINVER 0x601
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x601
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x601
#endif

#define FOOBAR2000_TARGET_VERSION 81
#include <helpers/foobar2000+atl.h>
#include "version.h"

#define FB2K_console_print_v(...) if (cfg_verbose) ::console::print("(", COMPONENT_NAME_H, ") ", __VA_ARGS__)
#define FB2K_console_print_e(...) ::console::print("(", COMPONENT_NAME_H, ") ", __VA_ARGS__)
