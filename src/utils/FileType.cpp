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

#include "FileType.h"

#include <cstring>

namespace
{
	struct Extension
	{
		const char* txt;
		size_t length;
		utils::FileType type;
	};

	const Extension extensions[] ={
		{".jpg", 4, utils::FileType::JPEG},
		{".jpeg", 5, utils::FileType::JPEG},
		{".png", 4, utils::FileType::PNG},
		{".tga", 4, utils::FileType::TGA},
		{".tiff", 5, utils::FileType::TIFF},
		{".gif", 4, utils::FileType::GIF},
		{".bmp", 4, utils::FileType::BMP},
		{".cbz", 4, utils::FileType::CBZ},
		{".cbr", 4, utils::FileType::CBR},
		{".cb7", 4, utils::FileType::CB7},
		{".cbt", 4, utils::FileType::CBT},
		{".pdf", 4, utils::FileType::PDF},
		{".xps", 4, utils::FileType::XPS},
		{".epub", 5, utils::FileType::EPUB}
	};

	const int nbExtensions = sizeof (extensions) / sizeof (extensions[0]);
}

namespace utils
{
	FileType getFileType(const char* fileName)
	{
		FileType type = FileType::UNKNOWN;
		if (fileName && (fileName[0] != '\0'))
		{
			size_t length = std::strlen(fileName);
			for (int i = 0; i < nbExtensions; ++i)
			{
				const Extension& ext = extensions[i];
				if ((length > ext.length) && !strcasecmp(fileName + length - ext.length, ext.txt))
				{
					type = ext.type;
					break;
				}
			}
		}

		return type;
	}

	bool isImageType(FileType type)
	{
		switch (type)
		{
		case FileType::JPEG:
		case FileType::PNG:
		case FileType::TGA:
		case FileType::TIFF:
		case FileType::GIF:
		case FileType::BMP:
			return true;

		default:
			return false;
		}
	}

	bool isComicBookType(FileType type)
	{
		switch (type)
		{
		case FileType::CBZ:
		case FileType::CBR:
		case FileType::CB7:
		case FileType::CBT:
			return true;

		default:
			return false;
		}
	}

	bool isDocumentType(FileType type)
	{
		switch (type)
		{
		case FileType::PDF:
		case FileType::XPS:
		case FileType::EPUB:
			return true;

		default:
			return false;
		}
	}
}
