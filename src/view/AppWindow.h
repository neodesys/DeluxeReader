/**
 * DeluxeReader
 *
 * Copyright (C) 2015, Loïc Le Page
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _APPWINDOW_H_
#define _APPWINDOW_H_

#if !defined(__linux__) && !defined(_WIN32)
#error Unsupported platform: currently supported platforms are\
       Linux and Windows
#endif //!__linux__ && !_WIN32

#include <cstddef>

#include "../utils/Logger.h"

namespace utils
{
	struct Icon;
}

namespace view
{
	class IWindowEventListener;

	struct Size
	{
		unsigned int uWidth;
		unsigned int uHeight;
	};

	class AppWindow final
	{
	public:
		~AppWindow();

		static AppWindow& getAppWindow()
		{
			return s_uniqueInstance;
		}

		bool create(int x = 0, int y = 0, int w = 0, int h = 0, const char* name = nullptr, const utils::Icon* icon = nullptr, bool bResizable = true);
		void destroy();

		bool isResizable() const
		{
			return m_bResizable;
		}

		bool isCursorVisible() const
		{
			return m_bCursorVisible;
		}

		bool showCursor(bool bVisible);

		bool isFullscreen() const
		{
			return m_bFullscreen;
		}

		bool setFullscreen(bool bFullscreen);

		bool isActivated() const
		{
			return m_bActivated;
		}

		const Size& getSize() const
		{
			return m_size;
		}

		void addWindowEventListener(IWindowEventListener& l);
		void removeWindowEventListener(IWindowEventListener& l);

		size_t getWindowEventListenerCount() const
		{
			return m_windowEventListenerCount;
		}

		bool dispatchWindowsEvents();

	private:
		AppWindow() = default;
		AppWindow(const AppWindow&) = delete;
		AppWindow& operator=(const AppWindow&) = delete;

		static AppWindow s_uniqueInstance;
		static const utils::Logger s_logger;

		bool m_bResizable = true;
		bool m_bCursorVisible = true;
		bool m_bFullscreen = false;
		bool m_bActivated = false;

		Size m_size = {};

		size_t m_windowEventListenerCount = 0;
		IWindowEventListener** m_windowEventListenerSet = nullptr;

		void fireRepaintEvent();
		void fireSizeChangedEvent(int w, int h);
		void fireStateChangedEvent(bool bActivated);
		void fireClosingEvent();
		void fireKeyPressedEvent(unsigned long key);
		void fireKeyReleasedEvent(unsigned long key);
		void fireButtonPressedEvent(unsigned int button, int x, int y);
		void fireButtonReleasedEvent(unsigned int button, int x, int y);
		void fireMouseWheelEvent(bool bUp, int x, int y);
		void firePointerMovedEvent(int x, int y);
	};
}

#endif //_APPWINDOW_H_
