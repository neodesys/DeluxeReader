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

#include "AppWindow.h"

#include <cstring>
#include <new>

#include "IWindowEventListener.h"

namespace view
{
	AppWindow AppWindow::s_uniqueInstance;

	const utils::Logger AppWindow::s_logger("view::AppWindow");

	AppWindow::~AppWindow()
	{
		destroy();

		m_windowEventListenerCount = 0;
		if (m_windowEventListenerSet)
		{
			delete[] m_windowEventListenerSet;
			m_windowEventListenerSet = nullptr;
		}
	}

	void AppWindow::addWindowEventListener(IWindowEventListener& l)
	{
		if (m_windowEventListenerSet)
		{
			IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
			for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
			{
				if (*p == &l)
				{
					s_logger.warning("window event listener already registered");
					return;
				}
			}
		}

		IWindowEventListener** newSet = new(std::nothrow) IWindowEventListener*[m_windowEventListenerCount + 1];
		if (!newSet)
		{
			s_logger.warning("cannot register a window event listener, out of memory");
			return;
		}

		if (m_windowEventListenerSet)
		{
			std::memcpy(newSet, m_windowEventListenerSet, m_windowEventListenerCount * sizeof(IWindowEventListener*));
			delete[] m_windowEventListenerSet;
		}

		m_windowEventListenerSet = newSet;
		m_windowEventListenerSet[m_windowEventListenerCount++] = &l;
	}

	void AppWindow::removeWindowEventListener(IWindowEventListener& l)
	{
		if (m_windowEventListenerSet)
		{
			if (m_windowEventListenerCount == 1)
			{
				if (*m_windowEventListenerSet == &l)
				{
					m_windowEventListenerCount = 0;
					delete[] m_windowEventListenerSet;
					m_windowEventListenerSet = nullptr;
					return;
				}
			}
			else
			{
				IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
				for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
				{
					if (*p == &l)
					{
						IWindowEventListener** newSet = new(std::nothrow) IWindowEventListener*[m_windowEventListenerCount - 1];
						if (!newSet)
						{
							s_logger.warning("cannot unregister a window event listener, out of memory");
							return;
						}

						IWindowEventListener** q = m_windowEventListenerSet;
						IWindowEventListener** r = newSet;
						while (q < p)
							*r++ = *q++;

						++q;
						while (q < pEnd)
							*r++ = *q++;

						delete[] m_windowEventListenerSet;
						m_windowEventListenerSet = newSet;
						--m_windowEventListenerCount;

						return;
					}
				}
			}
		}

		s_logger.warning("trying to unregister a non-registered window event listener");
	}

	void AppWindow::fireRepaintEvent()
	{
		if (m_windowEventListenerSet)
		{
			IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
			for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
				(*p)->onRepaint(*this);
		}
	}

	void AppWindow::fireSizeChangedEvent(int w, int h)
	{
		Size size = {1, 1};
		if (w > 1)
			size.uWidth = w;

		if (h > 1)
			size.uHeight = h;

		if ((size.uWidth != m_size.uWidth) || (size.uHeight != m_size.uHeight))
		{
			m_size = size;

			if (m_windowEventListenerSet)
			{
				IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
				for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
					(*p)->onSizeChanged(*this, m_size);
			}
		}
	}

	void AppWindow::fireStateChangedEvent(bool bActivated)
	{
		if (bActivated != m_bActivated)
		{
			m_bActivated = bActivated;

			if (m_windowEventListenerSet)
			{
				IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
				for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
					(*p)->onStateChanged(*this, m_bActivated);
			}
		}
	}

	void AppWindow::fireClosingEvent()
	{
		if (m_windowEventListenerSet)
		{
			IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
			for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
				(*p)->onClosing(*this);
		}
	}

	void AppWindow::fireKeyPressedEvent(unsigned long key)
	{
		if (m_windowEventListenerSet)
		{
			IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
			for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
				(*p)->onKeyPressed(*this, key);
		}
	}

	void AppWindow::fireKeyReleasedEvent(unsigned long key)
	{
		if (m_windowEventListenerSet)
		{
			IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
			for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
				(*p)->onKeyReleased(*this, key);
		}
	}

	void AppWindow::fireButtonPressedEvent(unsigned int button, int x, int y)
	{
		if (m_windowEventListenerSet)
		{
			IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
			for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
				(*p)->onButtonPressed(*this, button, x, y);
		}
	}

	void AppWindow::fireButtonReleasedEvent(unsigned int button, int x, int y)
	{
		if (m_windowEventListenerSet)
		{
			IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
			for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
				(*p)->onButtonReleased(*this, button, x, y);
		}
	}

	void AppWindow::fireMouseWheelEvent(bool bUp, int x, int y)
	{
		if (m_windowEventListenerSet)
		{
			IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
			for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
				(*p)->onMouseWheel(*this, bUp, x, y);
		}
	}

	void AppWindow::firePointerMovedEvent(int x, int y)
	{
		if (m_windowEventListenerSet)
		{
			IWindowEventListener** pEnd = m_windowEventListenerSet + m_windowEventListenerCount;
			for (IWindowEventListener** p = m_windowEventListenerSet; p < pEnd; ++p)
				(*p)->onPointerMoved(*this, x, y);
		}
	}
}
