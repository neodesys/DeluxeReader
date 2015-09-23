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

#ifndef _ARCHIVEBOOK_H_
#define _ARCHIVEBOOK_H_

#include "IBook.h"

struct archive;
struct archive_entry;

namespace bookcase
{
	class ArchiveBook final : public IBook
	{
	public:
		ArchiveBook() = default;
		ArchiveBook(const char* fileName);
		virtual ~ArchiveBook() override final;

		virtual bool open(const char* fileName) override final;
		virtual void close() override final;
		virtual bool isOpen() const override final;

		virtual bool goToNextEntry() override final;
		virtual bool goToEntry(int index) override final;

		virtual int getEntryIndex() const override final;
		virtual const char* getEntryPath() const override final;
		virtual bool getEntryData(BinBuffer& data) const override final;

	private:
		ArchiveBook(const ArchiveBook&) = delete;
		ArchiveBook& operator=(const ArchiveBook&) = delete;

		const char* m_fileName = nullptr;
		archive* m_pArchive = nullptr;
		archive_entry* m_pEntry = nullptr;
		int m_entryIdx = -1;

		bool openArchiveStream();
		void closeArchiveStream();
	};
}

#endif //_ARCHIVEBOOK_H_
