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

#include "BinaryBuffer.h"

#include <cstdlib>

namespace utils
{
	BinaryBuffer::~BinaryBuffer()
	{
		clear();
	}

	void BinaryBuffer::clear()
	{
		if (m_data)
		{
			//m_data has been allocated with C-style malloc, calloc or realloc
			std::free(m_data);
			m_data = nullptr;
		}

		m_size = 0;
	}

	void BinaryBuffer::setBuffer(unsigned char* data, size_t size)
	{
		if (data != m_data)
		{
			if (m_data)
				std::free(m_data);

			m_data = data;

			if (m_data)
				m_size = size;
			else
				m_size = 0;
		}
	}
}
