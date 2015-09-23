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

#ifndef _BINARYBUFFER_H_
#define _BINARYBUFFER_H_

#include <cstddef>

namespace utils
{
	//This class is used as a wrapper around a C buffer of bytes (allocated
	//with malloc, calloc or realloc)
	class BinaryBuffer
	{
	public:
		BinaryBuffer() = default;
		virtual ~BinaryBuffer();

		const unsigned char* getData() const
		{
			return m_data;
		}

		unsigned char* getData()
		{
			return m_data;
		}

		size_t getSize() const
		{
			return m_size;
		}

		void clear();

	protected:
		//The data buffer passed to this method MUST have been allocated with
		//a C memory function like malloc, calloc or realloc. Any previous
		//data is freed.
		void setBuffer(unsigned char* data, size_t size);

	private:
		BinaryBuffer(const BinaryBuffer&) = delete;
		BinaryBuffer& operator=(const BinaryBuffer&) = delete;

		unsigned char* m_data = nullptr;
		size_t m_size = 0;
	};
}

#endif //_BINARYBUFFER_H_
