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

#include "ImageIO.h"

#include "ImageIO_jpeg.h"
#include "ImageIO_png.h"
#include "ImageIO_tga.h"
#include "ImageIO_tiff.h"
#include "ImageIO_gif.h"
#include "ImageIO_bmp.h"

namespace
{
	utils::internal::ImageIO_jpeg jpegIO;
	utils::internal::ImageIO_png pngIO;
	utils::internal::ImageIO_tga tgaIO;
	utils::internal::ImageIO_tiff tiffIO;
	utils::internal::ImageIO_gif gifIO;
	utils::internal::ImageIO_bmp bmpIO;
}

namespace utils
{
	namespace internal
	{
		ImageIO* ImageIO::getImageIO(FileType type)
		{
			switch (type)
			{
			case FileType::JPEG: return &jpegIO;
			case FileType::PNG: return &pngIO;
			case FileType::TGA: return &tgaIO;
			case FileType::TIFF: return &tiffIO;
			case FileType::GIF: return &gifIO;
			case FileType::BMP: return &bmpIO;
			default: return nullptr;
			}
		}
	}
}
