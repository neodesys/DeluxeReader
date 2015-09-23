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

#ifndef _PDFBOOK_H_
#define	_PDFBOOK_H_

#include "IBook.h"

struct fz_context_s;
struct fz_document_s;

namespace bookcase
{
	class PDFBook final : public IBook
	{
	public:
		PDFBook() = default;
		PDFBook(const char* fileName);
		virtual ~PDFBook() override final;

		virtual bool open(const char* fileName) override final;
		virtual void close() override final;
		virtual bool isOpen() const override final;

		virtual bool goToNextEntry() override final;
		virtual bool goToEntry(int index) override final;

		virtual int getEntryIndex() const override final;
		virtual const char* getEntryPath() const override final;
		virtual bool getEntryData(BinBuffer& data) const override final;

	private:
		PDFBook(const PDFBook&) = delete;
		PDFBook& operator=(const PDFBook&) = delete;

		fz_context_s* m_pContext = nullptr;
		fz_document_s* m_pDocument = nullptr;
		int m_nbPages = 0;
		int m_entryIdx = -1;
	};
}

#endif //_PDFBOOK_H_
