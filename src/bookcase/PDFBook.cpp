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

#include "PDFBook.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef __cplusplus
extern "C" {
#endif
#include <mupdf/fitz.h>
#ifdef __cplusplus
}
#endif

namespace bookcase
{
	PDFBook::PDFBook(const char* fileName)
	{
		open(fileName);
	}

	PDFBook::~PDFBook()
	{
		close();
	}

	bool PDFBook::open(const char* fileName)
	{
		if (m_pContext || !fileName || (fileName[0] == '\0'))
			return false;

		assert(!m_pDocument);

		bool bRet = false;
		m_pContext = fz_new_context(nullptr, nullptr, FZ_STORE_UNLIMITED);
		if (m_pContext)
		{
			fz_try(m_pContext)
			{
				fz_register_document_handlers(m_pContext);
				m_pDocument = fz_open_document(m_pContext, fileName);
				if (m_pDocument)
				{
					m_nbPages = fz_count_pages(m_pDocument);
					if (m_nbPages > 0)
					{
						bRet = true;
						s_logger.info("opening \"%s\" document, %d page(s)", fileName, m_nbPages);
					}
					else
						s_logger.error("\"%s\" is an empty document", fileName);
				}
			}
			fz_catch(m_pContext)
			{
				s_logger.error("cannot open \"%s\": %s", fz_caught_message(m_pContext));
			}

			if (!bRet)
			{
				if (m_pDocument)
				{
					fz_close_document(m_pDocument);
					m_pDocument = nullptr;
				}

				fz_free_context(m_pContext);
				m_pContext = nullptr;
				m_nbPages = 0;
			}
		}

		return bRet;
	}

	void PDFBook::close()
	{
		if (m_pDocument)
		{
			fz_close_document(m_pDocument);
			m_pDocument = nullptr;
		}

		if (m_pContext)
		{
			fz_free_context(m_pContext);
			m_pContext = nullptr;
		}

		m_nbPages = 0;
		m_entryIdx = -1;
	}

	bool PDFBook::isOpen() const
	{
		return m_pDocument ? true : false;
	}

	bool PDFBook::goToNextEntry()
	{
		if (!m_pDocument || (m_entryIdx >= m_nbPages - 1))
			return false;

		++m_entryIdx;
		return true;
	}

	bool PDFBook::goToEntry(int index)
	{
		if (!m_pDocument || (index < 0) || (index >= m_nbPages))
			return false;

		m_entryIdx = index;
		return true;
	}

	int PDFBook::getEntryIndex() const
	{
		return m_entryIdx;
	}

	const char* PDFBook::getEntryPath() const
	{
		if (m_pDocument && (m_entryIdx >= 0))
		{
			assert(m_pContext);
			assert(m_entryIdx < m_nbPages);

			static char path[] = "rawRGBA_xxxx";
			if (std::snprintf(path + 8, 5, "%04d", m_entryIdx) == 4)
				return path;
		}

		return nullptr;
	}

	bool PDFBook::getEntryData(IBook::BinBuffer& data) const
	{
		bool bRet = false;
		if (m_pDocument && (m_entryIdx >= 0))
		{
			assert(m_pContext);
			assert(m_entryIdx < m_nbPages);

			fz_page* pPage = nullptr;
			fz_pixmap* pPixmap = nullptr;
			fz_device* pDevice = nullptr;

			fz_var(pPage);
			fz_var(pPixmap);
			fz_var(pDevice);

			fz_try(m_pContext)
			{
				pPage = fz_load_page(m_pDocument, m_entryIdx);
				if (pPage)
				{
					//Default output resolution of muPDF is 72 dpi
					fz_matrix transform = fz_identity;
					fz_pre_scale(&transform, 150.f / 72.f, 150.f / 72.f);

					fz_rect bounds = {};
					fz_bound_page(m_pDocument, pPage, &bounds);
					fz_transform_rect(&bounds, &transform);

					fz_irect bbox = {};
					fz_round_rect(&bbox, &bounds);

					pPixmap = fz_new_pixmap_with_bbox(m_pContext, fz_device_rgb(m_pContext), &bbox);
					if (pPixmap)
					{
						fz_clear_pixmap_with_value(m_pContext, pPixmap, 0xff);

						pDevice = fz_new_draw_device(m_pContext, pPixmap);
						if (pDevice)
						{
							fz_run_page(m_pDocument, pPage, pDevice, &transform, nullptr);
							fz_free_device(pDevice);
							pDevice = nullptr;

							if ((pPixmap->n == 4) && pPixmap->samples)
							{
								size_t size = 4 * pPixmap->w * pPixmap->h;
								if (size)
								{
									unsigned char* buffer = static_cast<unsigned char*>(std::malloc(size));
									if (buffer)
									{
										std::memcpy(buffer, pPixmap->samples, size);
										data.setBuffer(buffer, size);
										bRet = true;
										s_logger.info("page (idx: %d) extracted", m_entryIdx);
									}
									else
										s_logger.error("cannot allocate buffer for page data (idx: %d), out of memory", m_entryIdx);
								}
								else
									s_logger.error("invalid page size (idx: %d)", m_entryIdx);
							}
							else
								s_logger.error("invalid page format (idx: %d)", m_entryIdx);
						}
						else
							s_logger.error("cannot initialize page renderer (idx: %d)", m_entryIdx);

						fz_drop_pixmap(m_pContext, pPixmap);
						pPixmap = nullptr;
					}
					else
						s_logger.error("cannot create page rendering pixmap (idx: %d)", m_entryIdx);

					fz_free_page(m_pDocument, pPage);
					pPage = nullptr;
				}
				else
					s_logger.error("cannot parse document page (idx: %d)", m_entryIdx);
			}
			fz_catch(m_pContext)
			{
				s_logger.error("cannot open page (idx: %d): %s", m_entryIdx, fz_caught_message(m_pContext));

				if (pDevice)
					fz_free_device(pDevice);

				if (pPixmap)
					fz_drop_pixmap(m_pContext, pPixmap);

				if (pPage)
					fz_free_page(m_pDocument, pPage);
			}
		}

		return bRet;
	}
}
