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

#ifndef _IWINDOWEVENTLISTENER_H_
#define	_IWINDOWEVENTLISTENER_H_

namespace view
{
	class AppWindow;
	struct Size;

	/**
	 * Any class which wants to receive events dispatched by the application
	 * window must implement this interface.<br>
	 * A window event listener can be attached to the application window using
	 * AppWindow::addWindowEventListener().
	 * @brief Interface for a window event listener.
	 */
	class IWindowEventListener
	{
	public:
		virtual ~IWindowEventListener() = default;

		/**
		 * Receives a <i>Repaint</i> event when the application window needs
		 * repainting.
		 * @param wnd the application window.
		 */
		virtual void onRepaint(AppWindow& wnd) = 0;

		/**
		 * Receives a <i>SizeChanged</i> event when the application window size
		 * has changed.
		 * @param wnd the application window.
		 * @param size the new size of the application window client area.<br>
		 * It is guaranteed that width and height are both superior or equal
		 * to 1.
		 */
		virtual void onSizeChanged(AppWindow& wnd, const Size& size) = 0;

		/**
		 * Receives a <i>StateChanged</i> event when the application window
		 * state has changed.
		 * @param wnd the application window.
		 * @param bActivated true if the application window is activated or
		 * false if not.<br>
		 * As a general matter, drawing to the application window should be
		 * paused while it is not activated.
		 */
		virtual void onStateChanged(AppWindow& wnd, bool bActivated) = 0;

		/**
		 * Receives a <i>Closing</i> event when the application window asks for
		 * closing.<br>
		 * Normally, an application should respond to this event by destroying
		 * the application window and thus terminate the application.
		 * @param wnd the application window.
		 */
		virtual void onClosing(AppWindow& wnd) = 0;

		/**
		 * Receives a <i>KeyPressed</i> event when a key has been pressed while
		 * the application window had focus.
		 * @param wnd the application window.
		 * @param key identifier of the pressed key.
		 */
		virtual void onKeyPressed(AppWindow& wnd, unsigned long key) = 0;

		/**
		 * Receives a <i>KeyReleased</i> event when a key has been released
		 * while the application window had focus.
		 * @param wnd the application window.
		 * @param key identifier of the released key.
		 */
		virtual void onKeyReleased(AppWindow& wnd, unsigned long key) = 0;

		/**
		 * Receives a <i>ButtonPressed</i> event when a mouse button has been
		 * pressed while the cursor was in the application window client area.
		 * @param wnd the application window.
		 * @param button identifier of the pressed button:
		 * <ul>
		 *   <li>1: for the left mouse button.</li>
		 *   <li>2: for the middle mouse button.</li>
		 *   <li>3: for the right mouse button.</li>
		 *   <li>&gt;=4: for any extra mouse button.</li>
		 * </ul>
		 * @param x position of the cursor in the window client area.
		 * @param y position of the cursor in the window client area.
		 */
		virtual void onButtonPressed(AppWindow& wnd, unsigned int button, int x, int y) = 0;

		/**
		 * Receives a <i>ButtonReleased</i> event when a mouse button has been
		 * released while the cursor was in the application window client area.
		 * @param wnd the application window.
		 * @param button identifier of the released button:
		 * <ul>
		 *   <li>1: for the left mouse button.</li>
		 *   <li>2: for the middle mouse button.</li>
		 *   <li>3: for the right mouse button.</li>
		 *   <li>&gt;=4: for any extra mouse button.</li>
		 * </ul>
		 * @param x position of the cursor in the window client area.
		 * @param y position of the cursor in the window client area.
		 */
		virtual void onButtonReleased(AppWindow& wnd, unsigned int button, int x, int y) = 0;

		/**
		 * Receives a <i>MouseWheel</i> event when the mouse wheel has been
		 * turned while the cursor was in the application window client area.
		 * @param wnd the application window.
		 * @param bUp true if the wheel turned up (back to front) or false if
		 * it turned backwards (front to back).
		 * @param x position of the cursor in the window client area.
		 * @param y position of the cursor in the window client area.
		 */
		virtual void onMouseWheel(AppWindow& wnd, bool bUp, int x, int y) = 0;

		/**
		 * Receives a <i>PointerMoved</i> event when the cursor has moved over
		 * the application window client area.
		 * @param wnd the application window.
		 * @param x position of the cursor in the window client area.
		 * @param y position of the cursor in the window client area.
		 */
		virtual void onPointerMoved(AppWindow& wnd, int x, int y) = 0;
	};
}

#endif //_IWINDOWEVENTLISTENER_H_
