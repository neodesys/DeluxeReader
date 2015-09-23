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

#include "Image.h"

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <new>

#include <sys/stat.h>

#ifdef __linux__
#include <unistd.h>
#elif defined(_WIN32)
#include <io.h>
#else
#error Unsupported platform: currently supported platforms are\
       Linux and Windows
#endif

#include "internal/ImageIO.h"

namespace
{
	const size_t MAX_FILENAME_LENGTH = 2 * 1024;
}

namespace utils
{
	Image::~Image()
	{
		destroy();
	}

	bool Image::create(ImageType type, int w, int h, ImageAlign align)
	{
		if (m_data)
		{
			s_logger.error("cannot create image, internal data already exist");
			return false;
		}

		if ((w < 1) || (h < 1))
		{
			s_logger.error("cannot create image, invalid dimensions");
			return false;
		}

		m_type = type;
		m_uWidth = w;
		m_uHeight = h;
		m_alignment = align;

		unsigned int n = 1;
		switch (type)
		{
		case ImageType::LUMINANCE:
			break;

		case ImageType::LUMINANCE_ALPHA:
			n = 2;
			break;

		case ImageType::RGB:
			n = 3;
			break;

		case ImageType::RGBA:
			n = 4;
			break;
		}

		m_stride = n * m_uWidth;
		switch (align)
		{
		case ImageAlign::PACKED:
			break;

		case ImageAlign::ALIGN_32BITS:
			if (m_stride & 3l)
			{
				m_stride &= ~3l;
				m_stride += 4l;
			}
			break;

		case ImageAlign::ALIGN_64BITS:
			if (m_stride & 7l)
			{
				m_stride &= ~7l;
				m_stride += 8l;
			}
			break;
		}

		//Using () after new[] operator implies that each element in the array
		//is value-initialized, which means 0 for an unsigned char.
		m_data = new(std::nothrow) unsigned char[m_stride * m_uHeight]();
		if (!m_data)
		{
			destroy();
			s_logger.error("cannot create image, out of memory");
			return false;
		}

		s_logger.info("image created: %dx%d", w, h);
		return true;
	}

	void Image::destroy()
	{
		if (m_data)
		{
			delete[] m_data;
			m_data = nullptr;
			s_logger.info("image destroyed");
		}

		m_type = ImageType::RGBA;
		m_uWidth = 0;
		m_uHeight = 0;
		m_alignment = ImageAlign::PACKED;
		m_stride = 0;
	}

	bool Image::importFromMemory(FileType type, const unsigned char* data, size_t size)
	{
		if (m_data)
		{
			s_logger.error("cannot import image from memory, internal data already exist");
			return false;
		}

		if (!data || !size)
		{
			s_logger.error("cannot import image from memory, invalid source data");
			return false;
		}

		internal::ImageIO* pConverter = internal::ImageIO::getImageIO(type);
		if (!pConverter)
		{
			s_logger.error("cannot import image from memory, unknown source format");
			return false;
		}

		if (!pConverter->decodeImageData(data, size, *this))
		{
			s_logger.error("cannot import image from memory, decoding error");
			return false;
		}

		s_logger.info("image imported from memory");
		return true;
	}

	bool Image::exportToMemory(FileType type, Image::BinBuffer& data) const
	{
		if (!m_data)
		{
			s_logger.error("cannot export image to memory, no internal data available");
			return false;
		}

		internal::ImageIO* pConverter = internal::ImageIO::getImageIO(type);
		if (!pConverter)
		{
			s_logger.error("cannot export image to memory, unknown destination format");
			return false;
		}

		unsigned char* buffer = nullptr;
		size_t size = 0;
		if (!pConverter->encodeImageData(*this, buffer, size))
		{
			assert(!buffer);
			s_logger.error("cannot export image to memory, encoding error");
			return false;
		}

		data.setBuffer(buffer, size);
		s_logger.info("image exported to memory");
		return true;
	}

	bool Image::loadFromFile(const char* fileName)
	{
		if (m_data)
		{
			s_logger.error("cannot load image from file \"%s\", internal data already exist", fileName);
			return false;
		}

		FileType type = getFileType(fileName);
		internal::ImageIO* pConverter = internal::ImageIO::getImageIO(type);
		if (!pConverter)
		{
			s_logger.error("cannot load image from file \"%s\", unknown source format", fileName);
			return false;
		}

		std::FILE* srcFile = std::fopen(fileName, "rb");
		if (!srcFile)
		{
			s_logger.error("cannot load image from file \"%s\", cannot open source file", fileName);
			return false;
		}

		if (!pConverter->decodeImageFile(srcFile, *this))
		{
			std::fclose(srcFile);
			s_logger.error("cannot load image from file \"%s\", decoding error", fileName);
			return false;
		}

		std::fclose(srcFile);
		s_logger.info("image loaded from file \"%s\"", fileName);
		return true;
	}

	bool Image::saveToFile(const char* fileName) const
	{
		if (!m_data)
		{
			s_logger.error("cannot save image to file \"%s\", no internal data available", fileName);
			return false;
		}

		FileType type = getFileType(fileName);
		internal::ImageIO* pConverter = internal::ImageIO::getImageIO(type);
		if (!pConverter)
		{
			s_logger.error("cannot save image to file \"%s\", unknown destination format", fileName);
			return false;
		}

		//As the buffer for the temporary filename is allocated on the stack,
		//we limit the length of the filename to prevent any stack overflow.
		//Default maximum length is 2048 one-byte characters.
		//You should adapt this value if your stack size is inferior to 4kB.
		const size_t size = strlen(fileName);
		if (size > MAX_FILENAME_LENGTH)
		{
			s_logger.error("cannot save image to file \"%s\", path length is too big", fileName);
			return false;
		}

		char* tempName = reinterpret_cast<char*>(alloca(size + 8));
		std::memcpy(tempName, fileName, size);
		std::memcpy(tempName + size, "_XXXXXX", 8);

		int fd = mkstemp(tempName);
		if (fd == -1)
		{
			s_logger.error("cannot save image to file \"%s\", impossible to create temporary file \"%s\"", fileName, tempName);
			return false;
		}

		std::FILE* destFile = fdopen(fd, "wb");
		if (!destFile)
		{
			close(fd);
			std::remove(tempName);
			s_logger.error("cannot save image to file \"%s\", cannot open destination file", fileName);
			return false;
		}

		if (!pConverter->encodeImageFile(*this, destFile))
		{
			std::fclose(destFile);
			std::remove(tempName);
			s_logger.error("cannot save image to file \"%s\", encoding error", fileName);
			return false;
		}

		std::fclose(destFile);

		mode_t fileMode = 0;
		struct stat info = {};
		if (!stat(fileName, &info))
			fileMode = info.st_mode;
		else
		{
			fileMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
			mode_t mask = umask(0);
			umask(mask);
			fileMode &= ~mask;
		}

		if (std::rename(tempName, fileName))
		{
			if (std::remove(fileName) || std::rename(tempName, fileName))
			{
				std::remove(tempName);
				s_logger.error("cannot save image to file \"%s\", cannot rename temporary file \"%s\"", fileName, tempName);
				return false;
			}
		}

		if (chmod(fileName, fileMode))
			s_logger.warning("cannot set access rights on file \"%s\" while saving image", fileName);

		s_logger.info("image saved to file \"%s\"", fileName);
		return true;
	}

	const Logger Image::s_logger("utils::Image");
}
