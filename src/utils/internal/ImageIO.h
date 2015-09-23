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

#ifndef _IMAGEIO_H_
#define _IMAGEIO_H_

#include <cstdio>

#include "../FileType.h"

namespace utils
{
	class Image;

	namespace internal
	{
		class ImageIO
		{
		public:
			static ImageIO* getImageIO(FileType type);

			virtual bool decodeImageData(const unsigned char* srcData, size_t srcSize, Image& destImg) = 0;

			//On success, destData has been allocated with C-style malloc.
			//Caller must free the memory using free(destData) when the buffer
			//is no longer in use.
			virtual bool encodeImageData(const Image& srcImg, unsigned char*& destData, size_t& destSize) = 0;

			virtual bool decodeImageFile(std::FILE* srcFile, Image& destImg) = 0;
			virtual bool encodeImageFile(const Image& srcImg, std::FILE* destFile) = 0;

		protected:
			ImageIO() = default;
			~ImageIO() = default;

		private:
			ImageIO(const ImageIO&) = delete;
			ImageIO& operator=(const ImageIO&) = delete;
		};
	}
}

#endif //_IMAGEIO_H_
