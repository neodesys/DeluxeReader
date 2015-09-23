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

#ifdef _WIN32

#include "AppWindow.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>

#include "../utils/Image.h"

namespace view
{
	bool AppWindow::create(int x, int y, int w, int h, const char* name, const utils::Icon* icon, bool bResizable)
	{
		//TODO: implement Win32 Window create
		return false;
	}

	void AppWindow::destroy()
	{
		//TODO: implement Win32 Window destroy
		m_bResizable = true;
		m_bCursorVisible = true;
		m_bFullscreen = false;
		m_bActivated = false;
		m_size = {};
	}

	bool AppWindow::showCursor(bool bVisible)
	{
		//TODO: implement Win32 Window showCursor
		return false;
	}

	bool AppWindow::setFullscreen(bool bFullscreen)
	{
		//TODO: implement Win32 Window setFullscreen
		return false;
	}

	bool AppWindow::dispatchWindowsEvents()
	{
		//TODO: implement Win32 Window dispatchWindowsEvents
		return false;
	}
}

#endif //_WIN32
