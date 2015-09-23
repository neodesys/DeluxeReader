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

#ifndef _FILETYPE_H_
#define _FILETYPE_H_

namespace utils
{
	enum struct FileType : unsigned char
	{
		JPEG,
		PNG,
		TGA,
		TIFF,
		GIF,
		BMP,

		CBZ,
		CBR,
		CB7,
		CBT,

		PDF,
		XPS,
		EPUB,
		UNKNOWN
	};

	FileType getFileType(const char* fileName);

	bool isImageType(FileType type);
	bool isComicBookType(FileType type);
	bool isDocumentType(FileType type);
}

#endif //_FILETYPE_H_
