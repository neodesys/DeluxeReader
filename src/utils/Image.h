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

#ifndef _IMAGE_H_
#define	_IMAGE_H_

#include "FileType.h"
#include "BinaryBuffer.h"
#include "Logger.h"

namespace utils
{
	enum struct ImageType : unsigned char
	{
		LUMINANCE,
		LUMINANCE_ALPHA,
		RGB,
		RGBA
	};

	enum struct ImageAlign : unsigned char
	{
		PACKED,
		ALIGN_32BITS,
		ALIGN_64BITS
	};

	class Image
	{
	public:
		Image() = default;
		virtual ~Image();

		//On successful creation, image internal data are initialized with 0
		//for all color components (fully transparent black pixels).
		bool create(ImageType type, int w, int h, ImageAlign align = ImageAlign::PACKED);
		void destroy();

		ImageType getType() const
		{
			return m_type;
		}

		unsigned int getWidth() const
		{
			return m_uWidth;
		}

		unsigned int getHeight() const
		{
			return m_uHeight;
		}

		ImageAlign getAlignment() const
		{
			return m_alignment;
		}

		size_t getStride() const
		{
			return m_stride;
		}

		const unsigned char* getData() const
		{
			return m_data;
		}

		unsigned char* getData()
		{
			return m_data;
		}

		//For multi-images formats like GIF or TIFF, only the first image is
		//imported.
		bool importFromMemory(FileType type, const unsigned char* data, size_t size);

		class BinBuffer;
		bool exportToMemory(FileType type, BinBuffer& data) const;

		class BinBuffer final : public BinaryBuffer
		{
			friend bool Image::exportToMemory(FileType, BinBuffer&) const;
		};

		bool loadFromFile(const char* fileName);
		bool saveToFile(const char* fileName) const;

	protected:
		static const Logger s_logger;

	private:
		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		ImageType m_type = ImageType::RGBA;
		unsigned int m_uWidth = 0;
		unsigned int m_uHeight = 0;
		ImageAlign m_alignment = ImageAlign::PACKED;
		size_t m_stride = 0;
		unsigned char* m_data = nullptr; //Top-to-bottom scanlines
	};

	struct Icon
	{
		unsigned int nbLayers;
		const Image* layers;
	};
}

#endif //_IMAGE_H_
