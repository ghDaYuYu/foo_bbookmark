#pragma once

#include <windef.h>
#include <wingdi.h>
#include <functional>

#include "SDK/ui_element.h"

namespace {

	class StyleManager {

	public:

		virtual ~StyleManager() {
			setChangeHandler([] {});
		}

		virtual bool isDark() const = 0;
		virtual COLORREF getTitleColor() const = 0;
		virtual COLORREF getBgColor() const = 0;
		virtual COLORREF getSelColor() const = 0;
		virtual COLORREF getHighColor() const = 0;
		virtual COLORREF getHotColor() const = 0;
		virtual LOGFONT getTitleFont() const = 0;
		virtual LOGFONT getListFont() const = 0;
		virtual LOGFONT getPlaylistFont() const = 0;

		void setChangeHandler(std::function<void()> handler) {
			changeHandler = handler;
		};

		void onChange(uint32_t chg = 0) {
			if (glb::g_guiLists.empty()) {
				return;
			}

			updateCache();
			if (changeHandler)
				changeHandler();
		}

	protected:
	
		virtual COLORREF defaultTitleColor() = 0;
		virtual COLORREF defaultBgColor() = 0;
		virtual COLORREF defaultSelColor() = 0;
		virtual COLORREF defaultHighColor() = 0;
		virtual COLORREF defaultHotColor() = 0;
		virtual LOGFONT defaultTitleFont() = 0;
		virtual LOGFONT defaultListFont() = 0;
		virtual LOGFONT defaultPlaylistFont() = 0;

		void updateCache() {
			cachedTitleColor = defaultTitleColor();
			cachedBgColor = defaultBgColor();
			cachedSelColor = defaultSelColor();
			cachedHighColor = defaultHighColor();
			cachedHotColor = defaultHotColor();
			cachedTitleFont = defaultTitleFont();
			cachedListFont = defaultListFont();
			cachedPlaylistFont = defaultPlaylistFont();
		}

	protected:
		COLORREF cachedTitleColor{};
		COLORREF cachedBgColor{};
		COLORREF cachedSelColor{};
		COLORREF cachedHighColor{};
		COLORREF cachedHotColor{};
		LOGFONT cachedTitleFont{};
		LOGFONT cachedListFont{};
		LOGFONT cachedPlaylistFont{};

	private:
		std::function<void()> changeHandler;

	};
}