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

#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif //_WIN32

#include "view/AppWindow.h"
#include "view/IWindowEventListener.h"
#include "utils/FileLogger.h"
#include "utils/Image.h"

#define KEY_ESCAPE 0xff1b
#define KEY_RETURN 0xff0d
#define KEY_SPACE 0x0020

namespace
{
	class WindowListener final : public view::IWindowEventListener
	{
	public:
		virtual void onRepaint(view::AppWindow&) override final
		{
			std::cout << "Repaint Event" << std::endl;
		}

		virtual void onSizeChanged(view::AppWindow&, const view::Size& size) override final
		{
			std::cout << "Resize Event: w=" << size.uWidth << " h=" << size.uHeight << std::endl;
		}

		virtual void onStateChanged(view::AppWindow&, bool bActivated) override final
		{
			std::cout << "State Changed Event: " << (bActivated ? "activated" : "paused") << std::endl;
		}

		virtual void onClosing(view::AppWindow& wnd) override final
		{
			std::cout << "Closing Event" << std::endl;
			wnd.destroy();
		}

		virtual void onKeyPressed(view::AppWindow& wnd, unsigned long key) override final
		{
			std::cout << "KeyPress Event: " << key << std::endl;

			switch (key)
			{
			case KEY_ESCAPE:
				wnd.destroy();
				break;

			case KEY_RETURN:
				wnd.setFullscreen(!wnd.isFullscreen());
				break;

			case KEY_SPACE:
				wnd.showCursor(!wnd.isCursorVisible());
				break;
			}
		}

		virtual void onKeyReleased(view::AppWindow&, unsigned long key) override final
		{
			std::cout << "KeyRelease Event: " << key << std::endl;
		}

		virtual void onButtonPressed(view::AppWindow&, unsigned int button, int x, int y) override final
		{
			std::cout << "ButtonPress Event: btn=" << button << ", x=" << x << ", y=" << y << std::endl;
		}

		virtual void onButtonReleased(view::AppWindow&, unsigned int button, int x, int y) override final
		{
			std::cout << "ButtonRelease Event: btn=" << button << ", x=" << x << ", y=" << y << std::endl;
		}

		virtual void onMouseWheel(view::AppWindow&, bool bUp, int x, int y) override final
		{
			std::cout << "MouseWheel Event: " << (bUp ? "up" : "down") << ", x=" << x << ", y=" << y << std::endl;
		}

		virtual void onPointerMoved(view::AppWindow&, int x, int y) override final
		{
			std::cout << "PointerMove Event: x=" << x << ", y=" << y << std::endl;
		}
	};
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#else
int main()
#endif //_WIN32
{
	//TODO: implement main
	utils::FileLogger fileLogger("/dev/stdout");
	fileLogger.setLogLevel(utils::LogLevel::INFO);
	utils::Logger::setLogFormatter(&fileLogger);

	utils::Image iconImage;
	iconImage.loadFromFile("ico.png");
	utils::Icon defaultIcon = {1, &iconImage};

	view::AppWindow& wnd = view::AppWindow::getAppWindow();
	WindowListener l;
	wnd.addWindowEventListener(l);
	if (wnd.create(150, 150, 640, 480, "DeluxeReader", &defaultIcon, true))
	{
		iconImage.destroy();
		while (wnd.dispatchWindowsEvents());
		wnd.destroy();
	}

	return 0;
}
