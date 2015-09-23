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

#include "ArchiveBook.h"

#include <cstdlib>
#include <cstring>
#include <cassert>

#include <archive.h>
#include <archive_entry.h>

namespace
{
	const size_t BLOCK_SIZE = 32768;
	const int MAX_RETRIES = 3;
}

namespace bookcase
{
	ArchiveBook::ArchiveBook(const char* fileName)
	{
		open(fileName);
	}

	ArchiveBook::~ArchiveBook()
	{
		close();
	}

	bool ArchiveBook::open(const char* fileName)
	{
		if (m_fileName || !fileName || (fileName[0] == '\0'))
			return false;

		m_fileName = strdup(fileName);
		if (!m_fileName)
		{
			s_logger.error("cannot open \"%s\", out of memory", fileName);
			return false;
		}

		if (!openArchiveStream())
		{
			std::free(const_cast<char*>(m_fileName));
			m_fileName = nullptr;
			return false;
		}

		s_logger.info("opening \"%s\" document", fileName);
		return true;
	}

	void ArchiveBook::close()
	{
		closeArchiveStream();

		if (m_fileName)
		{
			std::free(const_cast<char*>(m_fileName));
			m_fileName = nullptr;
		}
	}

	bool ArchiveBook::isOpen() const
	{
		return (m_fileName);
	}

	bool ArchiveBook::goToNextEntry()
	{
		if (!m_pArchive)
			return false;

		int ret = archive_read_next_header(m_pArchive, &m_pEntry);
		if (ret == ARCHIVE_RETRY)
		{
			for (int i = 0; (ret == ARCHIVE_RETRY) && (i < MAX_RETRIES); ++i)
				ret = archive_read_next_header(m_pArchive, &m_pEntry);
		}

		if ((ret == ARCHIVE_OK) || (ret == ARCHIVE_WARN))
		{
			assert(m_pEntry);
			++m_entryIdx;
			return true;
		}

		return false;
	}

	bool ArchiveBook::goToEntry(int index)
	{
		if ((index < 0) || (!m_pArchive && !openArchiveStream()))
			return false;

		if (m_entryIdx >= 0)
		{
			if (index == m_entryIdx)
				return true;

			if (index < m_entryIdx)
			{
				closeArchiveStream();
				if (!openArchiveStream())
					return false;
			}
			else
				index -= m_entryIdx + 1;
		}

		while (index-- >= 0)
		{
			if (!goToNextEntry())
				return false;
		}

		return true;
	}

	int ArchiveBook::getEntryIndex() const
	{
		return m_entryIdx;
	}

	const char* ArchiveBook::getEntryPath() const
	{
		if (m_pEntry)
			return archive_entry_pathname(m_pEntry);
		else
			return nullptr;
	}

	bool ArchiveBook::getEntryData(IBook::BinBuffer& data) const
	{
		if (!m_pEntry)
			return false;

		assert(m_entryIdx >= 0);

		unsigned char* destBuffer = nullptr;
		size_t destSize = 0;
		size_t destPos = 0;
		if (archive_entry_size_is_set(m_pEntry))
		{
			destSize = archive_entry_size(m_pEntry);
			if (destSize)
			{
				destBuffer = static_cast<unsigned char*>(std::malloc(destSize));
				if (!destBuffer)
				{
					s_logger.error("cannot allocate buffer for page data (idx: %d), out of memory", m_entryIdx);
					return false;
				}
			}
		}

		const unsigned char* srcBuffer = nullptr;
		size_t srcSize = 0;
		off_t srcPos = 0;

		bool hasData = true;
		int retries = 0;
		while (hasData)
		{
			switch (archive_read_data_block(m_pArchive, reinterpret_cast<const void**>(&srcBuffer), &srcSize, &srcPos))
			{
			case ARCHIVE_OK:
			case ARCHIVE_WARN:
				retries = 0;
				if (srcSize)
				{
					assert(srcBuffer);
					if (!destBuffer || (destPos + srcSize > destSize))
					{
						destSize = destPos + srcSize;
						if (!destBuffer)
							destBuffer = static_cast<unsigned char*>(std::malloc(destSize));
						else
							destBuffer = static_cast<unsigned char*>(std::realloc(destBuffer, destSize));

						if (!destBuffer)
						{
							s_logger.error("cannot allocate buffer for page data (idx: %d), out of memory", m_entryIdx);
							return false;
						}
					}

					std::memcpy(destBuffer + destPos, srcBuffer, srcSize);
					destPos += srcSize;
				}
				break;

			case ARCHIVE_EOF:
				hasData = false;
				break;

			case ARCHIVE_RETRY:
				if (++retries > MAX_RETRIES)
				{
					if (destBuffer)
						std::free(destBuffer);

					s_logger.error("cannot read document page (idx: %d): %s", m_entryIdx, archive_error_string(m_pArchive));
					return false;
				}
				break;

			case ARCHIVE_FAILED:
			case ARCHIVE_FATAL:
				if (destBuffer)
					std::free(destBuffer);

				s_logger.error("cannot read document page (idx: %d): %s", m_entryIdx, archive_error_string(m_pArchive));
				return false;
			}
		}

		data.setBuffer(destBuffer, destPos);

		s_logger.info("page (idx: %d) extracted", m_entryIdx);
		return true;
	}

	bool ArchiveBook::openArchiveStream()
	{
		if (m_pArchive || !m_fileName)
			return false;

		m_pArchive = archive_read_new();
		if (!m_pArchive)
		{
			s_logger.error("cannot open \"%s\", archive initialization failed", m_fileName);
			return false;
		}

		archive_read_support_filter_all(m_pArchive);

		archive_read_support_format_tar(m_pArchive);
		archive_read_support_format_7zip(m_pArchive);
		archive_read_support_format_rar(m_pArchive);
		archive_read_support_format_zip(m_pArchive);

		if (archive_read_open_filename(m_pArchive, m_fileName, BLOCK_SIZE) != ARCHIVE_OK)
		{
			s_logger.error("cannot open \"%s\": %s", m_fileName, archive_error_string(m_pArchive));

			archive_read_free(m_pArchive);
			m_pArchive = nullptr;
			return false;
		}

		return true;
	}

	void ArchiveBook::closeArchiveStream()
	{
		if (m_pArchive)
		{
			archive_read_free(m_pArchive);
			m_pArchive = nullptr;
		}

		m_entryIdx = -1;
		m_pEntry = nullptr;
	}
}
