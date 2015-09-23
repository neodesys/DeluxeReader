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

#ifdef __linux__

#include "AppWindow.h"

#include <cassert>
#include <csignal>
#include <new>

#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <GL/glx.h>

#include "../utils/Image.h"

namespace
{
	const long _NET_WM_STATE_REMOVE = 0l;
	const long _NET_WM_STATE_ADD = 1l;

	enum struct AtomID : unsigned int
	{
		WM_DELETE_WINDOW,
		_NET_WM_PING,
		WM_PROTOCOLS,
		_NET_WM_PID,
		_NET_WM_WINDOW_TYPE,
		_NET_WM_WINDOW_TYPE_NORMAL,
		_NET_WM_ICON,
		WM_STATE,
		_NET_WM_STATE,
		_NET_WM_STATE_FULLSCREEN
	};

	class DisplayDelegate final
	{
	public:
		DisplayDelegate();
		~DisplayDelegate();

		bool createAppWindow(int x, int y, unsigned int uWidth, unsigned int uHeight, const char* name, const utils::Icon* icon, bool bResizable);
		void destroyAppWindow();

		bool showCursor(bool bVisible) const;
		bool setFullscreen(bool bFullscreen) const;

		bool isAppWindowInNormalState() const;
		bool repaintAppWindow() const;

		int fetchNextAppEvent(XEvent& event) const;

		Atom getAtom(AtomID id) const
		{
			return m_atoms[static_cast<unsigned int>(id)];
		}

	private:
		DisplayDelegate(const DisplayDelegate&) = delete;
		DisplayDelegate& operator=(const DisplayDelegate&) = delete;

		static const utils::Logger s_logger;

		static volatile sig_atomic_t s_signalQuit;
		static void signalInterception(int signal);

		static const char* const s_atomsNames[];
		static const unsigned int s_nbAtoms;

		long m_appPID = 0;

		Display* m_pDisplay = nullptr;
		Atom m_atoms[static_cast<unsigned int>(AtomID::_NET_WM_STATE_FULLSCREEN) + 1] = {};
		Cursor m_invisibleCursor = None;

		GLXContext m_glCtx = nullptr;
		Colormap m_colormap = None;

		Window m_appWindow = None;
	} s_displayDelegate;

	DisplayDelegate::DisplayDelegate()
	{
		struct sigaction signalHandler = {};
		signalHandler.sa_handler = &DisplayDelegate::signalInterception;
		sigaction(SIGHUP, &signalHandler, nullptr);
		sigaction(SIGINT, &signalHandler, nullptr);
		sigaction(SIGQUIT, &signalHandler, nullptr);
		sigaction(SIGTERM, &signalHandler, nullptr);

		m_appPID = getpid();
	}

	DisplayDelegate::~DisplayDelegate()
	{
		destroyAppWindow();
	}

	const utils::Logger DisplayDelegate::s_logger("view::AppWindow_X11");

	volatile sig_atomic_t DisplayDelegate::s_signalQuit = 0;

	void DisplayDelegate::signalInterception(int)
	{
		s_signalQuit = 1;
	}

	const char* const DisplayDelegate::s_atomsNames[] = {
		"WM_DELETE_WINDOW",
		"_NET_WM_PING",
		"WM_PROTOCOLS",
		"_NET_WM_PID",
		"_NET_WM_WINDOW_TYPE",
		"_NET_WM_WINDOW_TYPE_NORMAL",
		"_NET_WM_ICON",
		"WM_STATE",
		"_NET_WM_STATE",
		"_NET_WM_STATE_FULLSCREEN"
	};

	const unsigned int DisplayDelegate::s_nbAtoms = sizeof(s_atomsNames) / sizeof(s_atomsNames[0]);

	bool DisplayDelegate::createAppWindow(int x, int y, unsigned int uWidth, unsigned int uHeight, const char* name, const utils::Icon* icon, bool bResizable)
	{
		assert(uWidth);
		assert(uHeight);

		if (m_appWindow)
		{
			s_logger.warning("application window already exists");
			return true;
		}

		Display* pDisplay = XOpenDisplay(nullptr);
		if (!pDisplay)
		{
			s_logger.error("cannot open X11 display");
			return false;
		}

		if (!XInternAtoms(pDisplay, const_cast<char**>(s_atomsNames), s_nbAtoms, False, m_atoms))
		{
			XCloseDisplay(pDisplay);
			s_logger.error("cannot initialize X11 atoms");
			return false;
		}

		char data[1] = {};
		Pixmap blank = XCreateBitmapFromData(pDisplay, DefaultRootWindow(pDisplay), data, 1, 1);
		if (!blank)
		{
			XCloseDisplay(pDisplay);
			s_logger.error("cannot create invisible cursor blank pixmap");
			return false;
		}

		XColor col = {};
		Cursor invisibleCursor = XCreatePixmapCursor(pDisplay, blank, blank, &col, &col, 0, 0);
		XFreePixmap(pDisplay, blank);

		if (!invisibleCursor)
		{
			XCloseDisplay(pDisplay);
			s_logger.error("cannot create invisible cursor");
			return false;
		}

		int glxAttrList[] = {
			GLX_RGBA, GLX_DOUBLEBUFFER,
			GLX_RED_SIZE, 8,
			GLX_GREEN_SIZE, 8,
			GLX_BLUE_SIZE, 8,
			GLX_ALPHA_SIZE, 8,
			GLX_DEPTH_SIZE, 24,
			None
		};

		XVisualInfo* pVisInfo = glXChooseVisual(pDisplay, DefaultScreen(pDisplay), glxAttrList);
		if (!pVisInfo)
		{
			XFreeCursor(pDisplay, invisibleCursor);
			XCloseDisplay(pDisplay);
			s_logger.error("cannot find OpenGL compatible visual");
			return false;
		}

		GLXContext glCtx = glXCreateContext(pDisplay, pVisInfo, nullptr, True);
		if (!glCtx)
		{
			XFree(pVisInfo);
			XFreeCursor(pDisplay, invisibleCursor);
			XCloseDisplay(pDisplay);
			s_logger.error("cannot create OpenGL context");
			return false;
		}

		Window rootWindow = RootWindow(pDisplay, pVisInfo->screen);
		Colormap colormap = XCreateColormap(pDisplay, rootWindow, pVisInfo->visual, AllocNone);
		if (!colormap)
		{
			glXDestroyContext(pDisplay, glCtx);
			XFree(pVisInfo);
			XFreeCursor(pDisplay, invisibleCursor);
			XCloseDisplay(pDisplay);
			s_logger.error("cannot create colormap");
			return false;
		}

		XSetWindowAttributes wndAttr = {};
		wndAttr.colormap = colormap;
		wndAttr.background_pixel = BlackPixel(pDisplay, pVisInfo->screen);
		wndAttr.event_mask = ExposureMask | StructureNotifyMask | PropertyChangeMask | KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

		unsigned long wndAttrMask = CWColormap | CWBackPixel | CWEventMask;

		Window wnd = XCreateWindow(pDisplay, rootWindow, x, y, uWidth, uHeight, 0, pVisInfo->depth, InputOutput, pVisInfo->visual, wndAttrMask, &wndAttr);
		if (!wnd)
		{
			XFreeColormap(pDisplay, colormap);
			glXDestroyContext(pDisplay, glCtx);
			XFree(pVisInfo);
			XFreeCursor(pDisplay, invisibleCursor);
			XCloseDisplay(pDisplay);
			s_logger.error("cannot create X11 window");
			return false;
		}

		XFree(pVisInfo);

		//Set window properties for correct integration with the Window Manager
		XSetWMProtocols(pDisplay, wnd, m_atoms, 2);
		XChangeProperty(pDisplay, wnd, m_atoms[static_cast<unsigned int>(AtomID::_NET_WM_PID)], XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(&m_appPID), 1);
		XChangeProperty(pDisplay, wnd, m_atoms[static_cast<unsigned int>(AtomID::_NET_WM_WINDOW_TYPE)], XA_ATOM, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(m_atoms + static_cast<unsigned int>(AtomID::_NET_WM_WINDOW_TYPE_NORMAL)), 1);

		XTextProperty nameProp = {};
		XTextProperty* pNameProp = nullptr;
		if (name)
		{
			if (Xutf8TextListToTextProperty(pDisplay, const_cast<char**>(&name), 1, XStdICCTextStyle, &nameProp) >= Success)
				pNameProp = &nameProp;
			else
				s_logger.warning("cannot set window title: \"%s\"", name);
		}

		XSizeHints* pSizeHints = XAllocSizeHints();
		if (pSizeHints)
		{
			pSizeHints->flags = USPosition;
			pSizeHints->x = x;
			pSizeHints->y = y;

			if (!bResizable)
			{
				pSizeHints->flags |= PMinSize | PMaxSize;
				pSizeHints->min_width = pSizeHints->max_width = uWidth;
				pSizeHints->min_height = pSizeHints->max_height = uHeight;
			}
		}
		else
			s_logger.warning("cannot set window size hints");

		XWMHints* pWMHints = XAllocWMHints();
		if (pWMHints)
		{
			pWMHints->flags = StateHint | InputHint;
			pWMHints->initial_state = NormalState;
			pWMHints->input = True;
		}
		else
			s_logger.warning("cannot set window state hints");

		XClassHint* pClassHint = XAllocClassHint();
		if (pClassHint)
		{
			pClassHint->res_name = const_cast<char*>(name);
			pClassHint->res_class = const_cast<char*>(name);
		}
		else
			s_logger.warning("cannot set window class hints");

		XSetWMProperties(pDisplay, wnd, pNameProp, pNameProp, nullptr, 0, pSizeHints, pWMHints, pClassHint);

		if (pClassHint)
			XFree(pClassHint);

		if (pWMHints)
			XFree(pWMHints);

		if (pSizeHints)
			XFree(pSizeHints);

		if (pNameProp)
			XFree(pNameProp->value);

		//Set window icon
		//X11 window icon format is a packed array of unsigned long.
		//All icon layers are packed and each layer is stored as follows:
		//[uWidth, uHeight, pixel[0], pixel[1] ... pixel[uWidth * uHeight - 1]]
		//Each pixel is encoded in ARGB order with 8 bits per component.
		//Each layer is indexed in top-to-bottom order.
		if (icon && icon->layers)
		{
			unsigned int uLength = 0;
			for (unsigned int i = 0; i < icon->nbLayers; ++i)
			{
				const utils::Image* pImg = icon->layers + i;
				unsigned int uSize = pImg->getWidth() * pImg->getHeight();
				if (uSize)
					uLength += 2 + uSize;
			}

			if (uLength)
			{
				unsigned long* pBuffer = new(std::nothrow) unsigned long[uLength];
				if (pBuffer)
				{
					unsigned long* pDest = pBuffer;
					for (unsigned int i = 0; i < icon->nbLayers; ++i)
					{
						const utils::Image* pImg = icon->layers + i;
						unsigned int uWidth = pImg->getWidth();
						unsigned int uHeight = pImg->getHeight();
						if (uWidth && uHeight)
						{
							s_logger.info("set window icon: %dx%d", uWidth, uHeight);

							*pDest++ = uWidth;
							*pDest++ = uHeight;

							const utils::ImageType imgType = pImg->getType();
							const unsigned char* pSrcData = pImg->getData();
							for (unsigned int j = 0; j < uHeight; ++j)
							{
								const unsigned char* pSrc = pSrcData;
								for (unsigned int k = 0; k < uWidth; ++k)
								{
									unsigned long color = 0;
									switch (imgType)
									{
									case utils::ImageType::LUMINANCE:
										color |= *pSrc << 16;
										color |= *pSrc << 8;
										color |= *pSrc++;
										color |= 0xFF000000;
										break;

									case utils::ImageType::LUMINANCE_ALPHA:
										color |= *pSrc << 16;
										color |= *pSrc << 8;
										color |= *pSrc++;
										color |= *pSrc++ << 24;
										break;

									case utils::ImageType::RGB:
										color |= *pSrc++ << 16;
										color |= *pSrc++ << 8;
										color |= *pSrc++;
										color |= 0xFF000000;
										break;

									case utils::ImageType::RGBA:
										color |= *pSrc++ << 16;
										color |= *pSrc++ << 8;
										color |= *pSrc++;
										color |= *pSrc++ << 24;
										break;
									}

									*pDest++ = color;
								}

								pSrcData += pImg->getStride();
							}
						}
					}

					XChangeProperty(pDisplay, wnd, m_atoms[static_cast<unsigned int>(AtomID::_NET_WM_ICON)], XA_CARDINAL, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(pBuffer), uLength);
					delete[] pBuffer;
				}
				else
					s_logger.warning("cannot set window icon, out of memory");
			}
		}

		//Display window on screen
		XMapRaised(pDisplay, wnd);
		XFlush(pDisplay);

		//Switching the window to fullscreen BEFORE it has been physically
		//mapped on screen leads to a bad computation of desktop dimensions (in
		//particular desktop menu bars are not taken into account).
		//In order to allow calling AppWindow::setFullscreen() right after
		//AppWindow::create() without artefacts, we wait here until a MapNotify
		//event is available in the events queue for our application window.
		XEvent e = {};
		XPeekIfEvent(pDisplay, &e, [](Display*, XEvent* pEvent, XPointer pWnd)
		{
			return ((pEvent->type == MapNotify) && (pEvent->xany.window == *(reinterpret_cast<Window*>(pWnd)))) ? True : False;
		}, reinterpret_cast<XPointer>(&wnd));

		glXMakeCurrent(pDisplay, wnd, glCtx);

		m_pDisplay = pDisplay;
		m_invisibleCursor = invisibleCursor;

		m_glCtx = glCtx;
		m_colormap = colormap;

		m_appWindow = wnd;

		s_logger.info("application window created: x=%d, y=%d, w=%d, h=%d", x, y, uWidth, uHeight);
		return true;
	}

	void DisplayDelegate::destroyAppWindow()
	{
		if (m_appWindow)
		{
			assert(m_pDisplay);
			assert(m_glCtx);

			glXMakeCurrent(m_pDisplay, None, nullptr);
			glXDestroyContext(m_pDisplay, m_glCtx);
			m_glCtx = nullptr;

			XDestroyWindow(m_pDisplay, m_appWindow);
			m_appWindow = None;

			assert(m_colormap);
			XFreeColormap(m_pDisplay, m_colormap);
			m_colormap = None;

			assert(m_invisibleCursor);
			XFreeCursor(m_pDisplay, m_invisibleCursor);
			m_invisibleCursor = None;

			XCloseDisplay(m_pDisplay);
			m_pDisplay = nullptr;

			s_logger.info("application window destroyed");
		}
	}

	bool DisplayDelegate::showCursor(bool bVisible) const
	{
		if (m_appWindow)
		{
			assert(m_pDisplay);
			assert(m_invisibleCursor);

			if (bVisible)
			{
				XUndefineCursor(m_pDisplay, m_appWindow);
				s_logger.info("cursor shown");
			}
			else
			{
				XDefineCursor(m_pDisplay, m_appWindow, m_invisibleCursor);
				s_logger.info("cursor hidden");
			}

			XFlush(m_pDisplay);
			return true;
		}
		else
			s_logger.error("cannot change cursor visibility, application window doesn't exist");

		return false;
	}

	bool DisplayDelegate::setFullscreen(bool bFullscreen) const
	{
		if (m_appWindow)
		{
			assert(m_pDisplay);

			XEvent e = {};
			e.xclient.type = ClientMessage;
			e.xclient.window = m_appWindow;
			e.xclient.message_type = m_atoms[static_cast<unsigned int>(AtomID::_NET_WM_STATE)];
			e.xclient.format = 32;
			e.xclient.data.l[0] = bFullscreen ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
			e.xclient.data.l[1] = m_atoms[static_cast<unsigned int>(AtomID::_NET_WM_STATE_FULLSCREEN)];

			if (XSendEvent(m_pDisplay, DefaultRootWindow(m_pDisplay), False, SubstructureRedirectMask | SubstructureNotifyMask, &e))
			{
				s_logger.info("window switched to %s mode", bFullscreen ? "fullscreen" : "normal");
				XFlush(m_pDisplay);
				return true;
			}
			else
				s_logger.error("cannot change fullscreen state, internal protocol error");
		}
		else
			s_logger.error("cannot change fullscreen state, application window doesn't exist");

		return false;
	}

	bool DisplayDelegate::isAppWindowInNormalState() const
	{
		if (m_appWindow)
		{
			assert(m_pDisplay);

			Atom propType = None;
			int format = 0;
			unsigned long nbItems = 0;
			unsigned long bytesAfter = 0;
			long* prop = nullptr;
			Atom wmStateAtom = m_atoms[static_cast<unsigned int>(AtomID::WM_STATE)];
			XGetWindowProperty(m_pDisplay, m_appWindow, wmStateAtom, 0, 1, False, wmStateAtom, &propType, &format, &nbItems, &bytesAfter, reinterpret_cast<unsigned char**>(&prop));
			if (prop)
			{
				if ((propType == wmStateAtom) && (format == 32) && (nbItems == 1))
				{
					if (prop[0] == NormalState)
					{
						XFree(prop);
						return true;
					}
				}
				else
					s_logger.warning("cannot fetch window state, internal protocol error");

				XFree(prop);
			}
			else
				s_logger.warning("cannot fetch window state, internal protocol error");
		}
		else
			s_logger.warning("cannot fetch window state, application window doesn't exist");

		return false;
	}

	bool DisplayDelegate::repaintAppWindow() const
	{
		if (m_appWindow)
		{
			assert(m_pDisplay);

			//Don't duplicate event if an Expose event is already in the queue.
			XEvent e = {};
			while (XCheckTypedWindowEvent(m_pDisplay, m_appWindow, Expose, &e))
			{
				if (e.xexpose.count == 0)
				{
					XPutBackEvent(m_pDisplay, &e);
					return true;
				}
			}

			e = {};
			e.xexpose.type = Expose;
			e.xexpose.window = m_appWindow;

			if (XSendEvent(m_pDisplay, m_appWindow, False, ExposureMask, &e))
			{
				XFlush(m_pDisplay);
				return true;
			}
			else
				s_logger.error("cannot repaint window, internal protocol error");
		}
		else
			s_logger.error("cannot repaint window, application window doesn't exist");

		return false;
	}

	int DisplayDelegate::fetchNextAppEvent(XEvent& event) const
	{
		if (!m_appWindow || s_signalQuit)
			return 0;

		assert(m_pDisplay);
		if (XPending(m_pDisplay) <= 0)
			return 1;

		XNextEvent(m_pDisplay, &event);
		if (event.xany.window == m_appWindow)
			return 2;

		return 3;
	}
}

namespace view
{
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
		XEvent event = {};
		for (;;)
		{
			switch (s_displayDelegate.fetchNextAppEvent(event))
			{
			case 0:
				return false;

			case 1:
				return true;

			case 2:
				switch (event.type)
				{
				case Expose:
					if ((event.xexpose.count == 0) && m_bActivated)
						fireRepaintEvent();
					break;

				case MapNotify:
					if (s_displayDelegate.isAppWindowInNormalState())
						fireStateChangedEvent(true);
					break;

				case UnmapNotify:
					fireStateChangedEvent(false);
					break;

				case ConfigureNotify:
					fireSizeChangedEvent(event.xconfigure.width, event.xconfigure.height);
					break;

				case PropertyNotify:
					//We need to track WM_STATE property changes because all
					//Window Managers are not fully ICCCM compliant. Some
					//of them don't unmap the window as they should when it
					//goes iconic.
					if (event.xproperty.atom && (event.xproperty.atom == s_displayDelegate.getAtom(AtomID::WM_STATE)))
					{
						if (s_displayDelegate.isAppWindowInNormalState())
						{
							if (!m_bActivated)
							{
								fireStateChangedEvent(true);

								//With some Window Managers, the Expose event
								//may be sent before changing the state, so the
								//window is not redrawn after being activated.
								//Example with Muffin (Cinnamon): minimize the
								//window (state is changed to
								//false/desactivated), then choose "maximize"
								//in the contextual menu of the menu bar icon
								//(the window is resized and exposed but doesn't
								//show on screen and is still not activated),
								//then show the window by clicking on the menu
								//bar icon (the window is activated and shown on
								//screen but doesn't receive any Expose event).
								//To be sure that the window will receive an
								//Expose event AFTER being activated, we
								//explicitly send a new Expose event manually.
								s_displayDelegate.repaintAppWindow();
							}
						}
						else
							fireStateChangedEvent(false);
					}
					break;

				case KeyPress:
					fireKeyPressedEvent(XLookupKeysym(&event.xkey, 0));
					break;

				case KeyRelease:
					fireKeyReleasedEvent(XLookupKeysym(&event.xkey, 0));
					break;

				case ButtonPress:
					if (event.xbutton.button == 4)
						fireMouseWheelEvent(true, event.xbutton.x, event.xbutton.y);
					else if (event.xbutton.button == 5)
						fireMouseWheelEvent(false, event.xbutton.x, event.xbutton.y);
					else
						fireButtonPressedEvent(event.xbutton.button, event.xbutton.x, event.xbutton.y);
					break;

				case ButtonRelease:
					if ((event.xbutton.button != 4) && (event.xbutton.button != 5))
						fireButtonReleasedEvent(event.xbutton.button, event.xbutton.x, event.xbutton.y);
					break;

				case MotionNotify:
					firePointerMovedEvent(event.xmotion.x, event.xmotion.y);
					break;

				case ClientMessage:
					if ((event.xclient.message_type == s_displayDelegate.getAtom(AtomID::WM_PROTOCOLS)) &&
						(event.xclient.format == 32) &&
						event.xclient.data.l[0])
					{
						Atom atom = static_cast<Atom>(event.xclient.data.l[0]);
						if (atom == s_displayDelegate.getAtom(AtomID::WM_DELETE_WINDOW))
							fireClosingEvent();
						else if (atom == s_displayDelegate.getAtom(AtomID::_NET_WM_PING))
						{
							Window rootWindow = DefaultRootWindow(event.xclient.display);
							event.xclient.window = rootWindow;
							XSendEvent(event.xclient.display, rootWindow, False, SubstructureRedirectMask | SubstructureNotifyMask, &event);
						}
					}
					break;
				}
				break;
			}
		}
	}
}

#endif //__linux__
