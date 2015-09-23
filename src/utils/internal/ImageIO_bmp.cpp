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

#include "ImageIO_bmp.h"

#include "../Image.h"

namespace utils
{
	namespace internal
	{
		bool ImageIO_bmp::decodeImageData(const unsigned char* srcData, size_t srcSize, Image& destImg)
		{
			//TODO: implement BMP decode data
			return false;
		}

		bool ImageIO_bmp::encodeImageData(const Image& srcImg, unsigned char*& destData, size_t& destSize)
		{
			//TODO: implement BMP encode data
			return false;
		}

		bool ImageIO_bmp::decodeImageFile(std::FILE* srcFile, Image& destImg)
		{
			//TODO: implement BMP decode file
			return false;
		}

		bool ImageIO_bmp::encodeImageFile(const Image& srcImg, std::FILE* destFile)
		{
			//TODO: implement BMP encode file
			return false;
		}
	}
}
