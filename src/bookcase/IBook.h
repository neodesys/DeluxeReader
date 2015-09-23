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

#ifndef _IBOOK_H_
#define _IBOOK_H_

#include "../utils/BinaryBuffer.h"
#include "../utils/Logger.h"

//TODO: unadapted design, refactor the whole IBook system

namespace bookcase
{
	class IBook
	{
	public:
		virtual ~IBook() = default;

		virtual bool open(const char* fileName) = 0;
		virtual void close() = 0;
		virtual bool isOpen() const = 0;

		virtual bool goToNextEntry() = 0;
		virtual bool goToEntry(int index) = 0;

		virtual int getEntryIndex() const = 0;
		virtual const char* getEntryPath() const = 0;

		class BinBuffer;
		virtual bool getEntryData(BinBuffer& data) const = 0;

		class BinBuffer final : public utils::BinaryBuffer
		{
			friend class PDFBook;
			friend class ArchiveBook;
		};

	protected:
		static const utils::Logger s_logger;
	};
}

#endif //_IBOOK_H_
