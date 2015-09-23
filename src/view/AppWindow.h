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

//TODO: AppWindow_Win32 import, clean and adapt!
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif //_WIN32

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

		/**
		 * Creates and displays the application window on screen.
		 * @param x the initial horizontal position of the upper-left corner of
		 * the window in screen coordinates (default is 0).
		 * @param y the initial vertical position of the upper-left corner of
		 * the window in screen coordinates (default is 0).
		 * @param w the initial width of the window client area. If negative or
		 * null, the window will be created with a default geometry of 320x240
		 * (default value).
		 * @param h the initial height of the window client area. If negative or
		 * null, the window will be created with a default geometry of 320x240
		 * (default value).
		 * @param name the window name or nullptr if no name (default value).
		 * @param icon the window icon or nullptr if no icon (default value).
		 * @param bResizable pass true if the window is resizable (default
		 * value), else pass false.
		 * @return true on success or false on error.
		 */
		bool create(int x = 0, int y = 0, int w = 0, int h = 0, const char* name = nullptr, const utils::Icon* icon = nullptr, bool bResizable = true);

		/**
		 * Destroys the application window and releases any resources
		 * associated with it.
		 */
		void destroy();

		bool isResizable() const
		{
			return m_bResizable;
		}

		/**
		 * @return true if the cursor is showing while hovering over the window
		 * client area or false if not.<br>
		 * By default, cursor is not visible.
		 */
		bool isCursorVisible() const
		{
			return m_bCursorVisible;
		}

		/**
		 * @param bVisible pass true to show the cursor while hovering over the
		 * window client area, or pass false to hide the cursor.
		 * @return true on success or false on error.
		 */
		bool showCursor(bool bVisible);

		/**
		 * @return true if the window is in full-screen mode, else returns
		 * false if it is in normal windowed mode.<br>
		 * By default, window is in normal windowed mode.
		 */
		bool isFullscreen() const
		{
			return m_bFullscreen;
		}

		/**
		 * Switches the window to full-screen or windowed mode.
		 * @param bFullscreen pass true to go full-screen or false to get back
		 * to normal windowed mode.
		 * @return true on success or false on error.
		 */
		bool setFullscreen(bool bFullscreen);

		/**
		 * @return true if the window is activated (normal state), or false if
		 * not (iconic state).<br>
		 * You must call dispatchWindowsEvents() function regularly
		 * to maintain this information up-to-date.<br>
		 * As a general matter, drawing to the window should be paused while
		 * the window is not activated.
		 */
		bool isActivated() const
		{
			return m_bActivated;
		}

		/**
		 * @return the current size of the window client area.<br>
		 * Once the window has been created, it is guaranteed that returned
		 * width and height are both superior or equal to 1.
		 * If the window has not been created yet, returned width and height
		 * are both null.<br>
		 * You must call dispatchWindowsEvents() function regularly to maintain
		 * this size up-to-date.
		 */
		const Size& getSize() const
		{
			return m_size;
		}

		/**
		 * Adds a window event listener to the window.
		 * @param l the window event listener to add.
		 */
		void addWindowEventListener(IWindowEventListener& l);

		/**
		 * Removes a window event listener from the window.
		 * @param l the window event listener to remove.
		 */
		void removeWindowEventListener(IWindowEventListener& l);

		size_t getWindowEventListenerCount() const
		{
			return m_windowEventListenerCount;
		}

		/**
		 * Window events dispatching method.<br>
		 * It must be called regularly in order to correctly dispatch all
		 * events to the application window.<br>
		 * In particular, window current size and state are updated during the
		 * event loop.
		 * @return true if the application window is still valid, or false if
		 * the application window has been destroyed or if the program has been
		 * killed by an external signal.<br>
		 * As a general matter, if this function returns false, the program
		 * should terminate.
		 */
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

		//TODO: AppWindow_Win32 import, clean and adapt!
#ifdef _WIN32
		friend LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif //_WIN32
	};
}

#endif //_APPWINDOW_H_
