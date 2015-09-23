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

//TODO: AppWindow_Win32 quick and dirty import from another project, must be cleaned and adapted

#include "AppWindow.h"

#include <csignal>

#include <windowsx.h>
#include <GL/gl.h>

#include "../utils/Image.h"

#ifndef ENDSESSION_CLOSEAPP
#define ENDSESSION_CLOSEAPP 0x1
#endif //!ENDSESSION_CLOSEAPP

namespace
{
	class DisplayDelegate final
	{
	public:
		DisplayDelegate();
		~DisplayDelegate();

		bool createAppWindow(int x, int y, unsigned int uWidth, unsigned int uHeight, const char* name, const utils::Icon* icon, bool bResizable);
		void destroyAppWindow();

		bool showCursor(bool bVisible) const;
		bool setFullscreen(bool bFullscreen) const;

		inline bool mustQuit() const
		{
			return s_signalQuit;
		}

		static inline void signalQuit()
		{
			s_signalQuit = 1;
		}

	private:
		DisplayDelegate(const DisplayDelegate&) = delete;
		DisplayDelegate& operator=(const DisplayDelegate&) = delete;

		static const utils::Logger s_logger;

		static volatile sig_atomic_t s_signalQuit;
		static BOOL WINAPI signalInterception(DWORD dwCtrlType);

		ATOM m_wndClassAtom = 0;
		HWND m_hAppWindow = NULL;
		HDC m_hDC = NULL;
		HGLRC m_hGLCtx = NULL;

		bool m_bTrackingMouse = false;

		bool initWindowClass();
	} s_displayDelegate;

	DisplayDelegate::DisplayDelegate()
	{
		SetConsoleCtrlHandler(&DisplayDelegate::signalInterception, TRUE);
	}

	DisplayDelegate::~DisplayDelegate()
	{
		destroyAppWindow();
	}

	const utils::Logger DisplayDelegate::s_logger("view::AppWindow_Win32");

	volatile sig_atomic_t DisplayDelegate::s_signalQuit = 0;

	BOOL WINAPI DisplayDelegate::signalInterception(DWORD)
	{
		s_signalQuit = 1;
		return TRUE;
	}

	bool DisplayDelegate::createAppWindow(int x, int y, unsigned int uWidth, unsigned int uHeight, const char* name, const utils::Icon* icon, bool bResizable)
	{
		//TODO
		if (m_hAppWindow || !initWindowClass())
			return false;

		RECT wndRect = {x, y, x + (int)uWidth, y + (int)uHeight};
		DWORD wndStyle = WS_OVERLAPPEDWINDOW;
		DWORD wndExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;

		wndStyle |= WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
		AdjustWindowRectEx(&wndRect, wndStyle, FALSE, wndExStyle);

		HWND hWnd = CreateWindowEx(wndExStyle, reinterpret_cast<LPCSTR>(m_wndClassAtom), name, wndStyle, wndRect.left, wndRect.top, wndRect.right - wndRect.left, wndRect.bottom - wndRect.top, NULL, NULL, GetModuleHandle(NULL), NULL);
		if (!hWnd)
			return false;

		HDC hDC = GetDC(hWnd);
		if (!hDC)
		{
			DestroyWindow(hWnd);
			return false;
		}

		static const PIXELFORMATDESCRIPTOR pfd = {
			sizeof(PIXELFORMATDESCRIPTOR), 1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			32,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			24,
			8,
			0, PFD_MAIN_PLANE, 0, 0, 0, 0
		};

		int pixFormat = ChoosePixelFormat(hDC, &pfd);
		if (!pixFormat || !SetPixelFormat(hDC, pixFormat, &pfd))
		{
			ReleaseDC(hWnd, hDC);
			DestroyWindow(hWnd);
			return false;
		}

		m_hGLCtx = wglCreateContext(hDC);
		if (!m_hGLCtx)
		{
			ReleaseDC(hWnd, hDC);
			DestroyWindow(hWnd);
			return false;
		}

		m_hDC = hDC;
		m_hAppWindow = hWnd;

		ShowWindow(hWnd, SW_SHOW);
		SetForegroundWindow(hWnd);
		SetFocus(hWnd);

		return true;
	}

	void DisplayDelegate::destroyAppWindow()
	{
		m_bTrackingMouse = false;

		if (m_hGLCtx)
		{
			wglMakeCurrent(NULL, NULL);
			wglDeleteContext(m_hGLCtx);
			m_hGLCtx = NULL;
		}

		if (m_hDC)
		{
			if (m_hAppWindow)
				ReleaseDC(m_hAppWindow, m_hDC);

			m_hDC = NULL;
		}

		if (m_hAppWindow)
		{
			DestroyWindow(m_hAppWindow);
			m_hAppWindow = NULL;

			ReleaseCapture();
			ClipCursor(NULL);
			ShowCursor(true);
		}

		if (m_wndClassAtom)
		{
			UnregisterClass(reinterpret_cast<LPCSTR>(m_wndClassAtom), NULL);
			m_wndClassAtom = 0;
		}
	}

	bool DisplayDelegate::showCursor(bool bVisible) const
	{
		//TODO
		if (m_hAppWindow)
		{
			if (bVisible)
			{
				if (m_bTrackingMouse)
				{
					m_bTrackingMouse = false;
					ShowCursor(true);

					TRACKMOUSEEVENT tme = {};
					tme.cbSize = sizeof(tme);
					tme.dwFlags = TME_CANCEL | TME_LEAVE;
					tme.hwndTrack = m_hAppWindow;
					TrackMouseEvent(&tme);
				}

				s_logger.info("cursor shown");
			}
			else
			{
				//Generate a WM_MOUSEMOVE event if cursor is within this
				//window client area.
				POINT p = {};
				if (GetCursorPos(&p))
					SetCursorPos(p.x, p.y);

				s_logger.info("cursor hidden");
			}

			return true;
		}
		else
			s_logger.error("cannot change cursor visibility, application window doesn't exist");

		return false;
	}

	bool DisplayDelegate::setFullscreen(bool bFullscreen) const
	{
		//TODO
		return false;
	}

	bool DisplayDelegate::initWindowClass()
	{
		if (m_wndClassAtom)
			return true;

		WNDCLASSEX wc = {};
		wc.cbSize = sizeof(wc);
		wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
		wc.lpfnWndProc = (WNDPROC)&view::windowProc;
		wc.hInstance = GetModuleHandle(NULL);
		wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wc.hCursor = (HCURSOR)LoadImage(NULL, IDC_ARROW, IMAGE_CURSOR, 0, 0, LR_SHARED | LR_DEFAULTSIZE);
		wc.lpszClassName = "DeluxeReader";

		m_wndClassAtom = RegisterClassEx(&wc);
		if (m_wndClassAtom)
			return true;
		else
			return false;
	}
}

namespace view
{
	LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		view::AppWindow& wnd = view::AppWindow::getAppWindow();
		switch (uMsg)
		{
		case WM_PAINT:
			wnd.fireRepaintEvent();
			break;

		case WM_ACTIVATE:
			if (wnd.isFullscreen())
			{
				if (LOWORD(wParam) == WA_INACTIVE)
					ShowWindow(hWnd, SW_MINIMIZE);
				else
				{
					//TODO: restore full-screen window on further activation
				}
			}
			return 0;

		case WM_WINDOWPOSCHANGED:
			{
				const WINDOWPOS* pWndPos = reinterpret_cast<const WINDOWPOS*>(lParam);
				if (pWndPos)
				{
					if (IsIconic(hWnd) || !IsWindowVisible(hWnd))
						wnd.fireStateChangedEvent(false);
					else
					{
						if (!(pWndPos->flags & SWP_NOSIZE))
							wnd.fireSizeChangedEvent(pWndPos->cx, pWndPos->cy);

						wnd.fireStateChangedEvent(true);
					}
				}
			}
			return 0;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			wnd.fireKeyPressedEvent(wParam);
			return 0;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			wnd.fireKeyReleasedEvent(wParam);
			return 0;

		case WM_LBUTTONDOWN:
			wnd.fireButtonPressedEvent(1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WM_LBUTTONUP:
			wnd.fireButtonReleasedEvent(1, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WM_MBUTTONDOWN:
			wnd.fireButtonPressedEvent(2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WM_MBUTTONUP:
			wnd.fireButtonReleasedEvent(2, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WM_RBUTTONDOWN:
			wnd.fireButtonPressedEvent(3, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WM_RBUTTONUP:
			wnd.fireButtonReleasedEvent(3, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;

		case WM_MOUSEWHEEL:
			{
				short delta = GET_WHEEL_DELTA_WPARAM(wParam);
				POINT pos = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
				if (ScreenToClient(hWnd, &pos))
				{
					if (delta > 0)
						wnd.fireMouseWheelEvent(true, pos.x, pos.y);
					else if (delta < 0)
						wnd.fireMouseWheelEvent(false, pos.x, pos.y);
				}
			}
			return 0;

		//TODO
		/*
		case WM_MOUSELEAVE:
			if (wnd.m_bTrackingMouse)
			{
				wnd.m_bTrackingMouse = false;

				if (!wnd.m_bCursorVisible && !wnd.isFullScreen())
					ShowCursor(true);
			}
			return 0;

		case WM_MOUSEMOVE:
			if (!wnd.m_bCursorVisible && !wnd.isFullScreen() && !wnd.m_bTrackingMouse)
			{
				TRACKMOUSEEVENT tme = {};
				tme.cbSize = sizeof(tme);
				tme.dwFlags = TME_LEAVE;
				tme.hwndTrack = hWnd;
				if (TrackMouseEvent(&tme))
				{
					wnd.m_bTrackingMouse = true;
					ShowCursor(false);
				}
			}

			wnd.firePointerMovedEvent(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
			return 0;
		*/

		case WM_CLOSE:
			wnd.fireClosingEvent();
			return 0;

		case WM_ENDSESSION:
			if ((lParam & ENDSESSION_CLOSEAPP) && !wParam)
				return 0;
			else
				DisplayDelegate::signalQuit();
			return 0;

		case WM_SYSCOMMAND:
			if (((wParam == SC_SCREENSAVE) || (wParam == SC_MONITORPOWER)) && wnd.isFullscreen())
				return 0;
			break;
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	bool AppWindow::create(int x, int y, int w, int h, const char* name, const utils::Icon* icon, bool bResizable)
	{
		if ((w < 1) || (h < 1))
		{
			w = 320;
			h = 240;
		}

		if (s_displayDelegate.createAppWindow(x, y, w, h, name, icon, bResizable))
		{
			m_bResizable = bResizable;
			return true;
		}
		else
			return false;
	}

	void AppWindow::destroy()
	{
		fireStateChangedEvent(false);
		s_displayDelegate.destroyAppWindow();
		m_bResizable = true;
		m_bCursorVisible = true;
		m_bFullscreen = false;
		m_bActivated = false;
		m_size = {};
	}

	bool AppWindow::showCursor(bool bVisible)
	{
		if (bVisible != m_bCursorVisible)
		{
			if (!s_displayDelegate.showCursor(bVisible))
				return false;

			m_bCursorVisible = bVisible;
		}

		return true;
	}

	bool AppWindow::setFullscreen(bool bFullscreen)
	{
		if (!m_bResizable)
		{
			s_logger.error("cannot change fullscreen state, application window is not resizable");
			return false;
		}

		if (bFullscreen != m_bFullscreen)
		{
			if (!s_displayDelegate.setFullscreen(bFullscreen))
				return false;

			m_bFullscreen = bFullscreen;
		}

		return true;
	}

	bool AppWindow::dispatchWindowsEvents()
	{
		MSG msg;
		for (;;)
		{
			if (s_displayDelegate.mustQuit())
				return false;

			if (!PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
				return true;

			if (msg.message == WM_QUIT)
			{
				DisplayDelegate::signalQuit();
				return false;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

#endif //_WIN32
