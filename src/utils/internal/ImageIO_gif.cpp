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

#include "ImageIO_gif.h"

#include <cstdlib>
#include <cstring>
#include <cassert>
#include <new>

#include <gif_lib.h>

#include "../Image.h"

namespace
{
	const unsigned char TRANSPARENCY_ALPHA_TRESHOLD = 127;

	const utils::Logger s_logger("utils::Image::Codec_GIF");

	void logLastGIFError()
	{
		const char* err = nullptr;
		int errCode = GifLastError();

		switch (errCode)
		{
		case E_GIF_ERR_OPEN_FAILED:
			err = "cannot open stream for writing";
			break;

		case E_GIF_ERR_WRITE_FAILED:
			err = "cannot write to stream";
			break;

		case E_GIF_ERR_HAS_SCRN_DSCR:
			err = "screen descriptor already set";
			break;

		case E_GIF_ERR_HAS_IMAG_DSCR:
			err = "image descriptor still active";
			break;

		case E_GIF_ERR_NO_COLOR_MAP:
		case D_GIF_ERR_NO_COLOR_MAP:
			err = "undefined colormap";
			break;

		case E_GIF_ERR_DATA_TOO_BIG:
		case D_GIF_ERR_DATA_TOO_BIG:
			err = "pixel buffer is too big, greater than width x height";
			break;

		case E_GIF_ERR_NOT_ENOUGH_MEM:
		case D_GIF_ERR_NOT_ENOUGH_MEM:
			err = "out of memory";
			break;

		case E_GIF_ERR_DISK_IS_FULL:
			err = "write failed, disk may be full";
			break;

		case E_GIF_ERR_CLOSE_FAILED:
		case D_GIF_ERR_CLOSE_FAILED:
			err = "cannot close stream";
			break;

		case E_GIF_ERR_NOT_WRITEABLE:
			err = "stream is not writable";
			break;

		case D_GIF_ERR_OPEN_FAILED:
			err = "cannot open stream for reading";
			break;

		case D_GIF_ERR_READ_FAILED:
			err = "cannot read from stream";
			break;

		case D_GIF_ERR_NOT_GIF_FILE:
			err = "invalid GIF format";
			break;

		case D_GIF_ERR_NO_SCRN_DSCR:
			err = "no screen descriptor found";
			break;

		case D_GIF_ERR_NO_IMAG_DSCR:
			err = "no image descriptor found";
			break;

		case D_GIF_ERR_WRONG_RECORD:
			err = "invalid record type";
			break;

		case D_GIF_ERR_NOT_READABLE:
			err = "stream is not readable";
			break;

		case D_GIF_ERR_IMAGE_DEFECT:
			err = "invalid image data, decoding aborted";
			break;

		case D_GIF_ERR_EOF_TOO_SOON:
			err = "stream EOF detected before image completion";
			break;
		}

		if (err)
			s_logger.error(err);
		else
			s_logger.error("unknown GIF error: %d", errCode);
	}

	class GIFReader final
	{
	public:
		GIFReader(const unsigned char* srcData, size_t srcSize);
		GIFReader(std::FILE* srcFile);

		bool isValid() const
		{
			return (m_srcFile || m_srcData);
		}

		static int readFunc(GifFileType* ctx, unsigned char* data, int size);

	private:
		GIFReader(const GIFReader&) = delete;
		GIFReader& operator=(const GIFReader&) = delete;

		int read(unsigned char* data, int size);

		const unsigned char* m_srcData = nullptr;
		size_t m_srcSize = 0;
		size_t m_pos = 0;

		std::FILE* m_srcFile = nullptr;
	};

	GIFReader::GIFReader(const unsigned char* srcData, size_t srcSize)
	{
		if (srcData && srcSize)
		{
			m_srcData = srcData;
			m_srcSize = srcSize;
		}
	}

	GIFReader::GIFReader(std::FILE* srcFile)
	{
		if (srcFile)
			m_srcFile = srcFile;
	}

	int GIFReader::readFunc(GifFileType* ctx, unsigned char* data, int size)
	{
		if (size > 0)
		{
			assert(ctx);
			GIFReader* pReader = reinterpret_cast<GIFReader*>(ctx->UserData);
			assert(pReader);
			return pReader->read(data, size);
		}
		else
			return 0;
	}

	int GIFReader::read(unsigned char* data, int size)
	{
		assert(data);
		assert(size > 0);

		if (m_srcFile)
			return std::fread(data, 1, size, m_srcFile);
		else if (m_srcData)
		{
			assert(m_srcSize);
			if (m_pos + size > m_srcSize)
			{
				size = m_srcSize - m_pos;
				if (size <= 0)
					return 0;
			}

			std::memcpy(data, m_srcData + m_pos, size);
			m_pos += size;
			return size;
		}
		else
		{
			s_logger.error("read error, invalid reader state");
			return 0;
		}
	}

	bool readGIF(GIFReader& reader, utils::Image& destImg)
	{
		if (destImg.getData() || !reader.isValid())
			return false;

		GifFileType* ctx = DGifOpen(&reader, &GIFReader::readFunc);
		if (!ctx)
		{
			logLastGIFError();
			return false;
		}

		if (DGifSlurp(ctx) == GIF_ERROR)
		{
			logLastGIFError();
			DGifCloseFile(ctx);
			return false;
		}

		if ((ctx->ImageCount <= 0) || !ctx->SavedImages)
		{
			DGifCloseFile(ctx);
			s_logger.error("invalid empty GIF image");
			return false;
		}

		//We only process the first sub-image in case of an animated GIF
		const SavedImage& subImg = ctx->SavedImages[0];

		int w = ctx->SWidth;
		int h = ctx->SHeight;
		int subLeft = subImg.ImageDesc.Left;
		int subWidth = subImg.ImageDesc.Width;
		int subTop = subImg.ImageDesc.Top;
		int subHeight = subImg.ImageDesc.Height;
		if ((w <= 0) || (h <= 0) ||
			(subLeft < 0) || (subTop < 0) ||
			(subWidth <= 0) || (subHeight <= 0) ||
			(subLeft + subWidth > w) || (subTop + subHeight > h))
		{
			DGifCloseFile(ctx);
			s_logger.error("invalid GIF image dimensions");
			return false;
		}

		GifColorType* pBackColor = nullptr;
		ColorMapObject* colormap = ctx->SColorMap;
		if (colormap)
		{
			//Background color is only used with a global colormap
			int backColorIdx = ctx->SBackGroundColor;
			if ((backColorIdx >= 0) && (backColorIdx < colormap->ColorCount) && colormap->Colors)
			{
				pBackColor = colormap->Colors + backColorIdx;

				//If background color is black, we discard it as destImg is
				//already initialized with black pixels on creation
				if (!pBackColor->Red && !pBackColor->Green && !pBackColor->Blue)
					pBackColor = nullptr;
			}
		}

		//If sub-image has a local colormap, use it in place of the global one
		if (subImg.ImageDesc.ColorMap)
			colormap = subImg.ImageDesc.ColorMap;

		if (!colormap || (colormap->ColorCount <= 0) || !colormap->Colors)
		{
			DGifCloseFile(ctx);
			s_logger.error("invalid GIF colormap");
			return false;
		}

		//Find transparency index if available
		int transColorIdx = -1;
		if (subImg.ExtensionBlocks)
		{
			for (int i = 0; i < subImg.ExtensionBlockCount; ++i)
			{
				const ExtensionBlock& ext = subImg.ExtensionBlocks[i];
				if ((ext.Function == GRAPHICS_EXT_FUNC_CODE) && (ext.ByteCount >= 4) && ext.Bytes)
				{
					//We found a "Graphics Control Extension" which defines
					//transparency settings and animation control
					if (ext.Bytes[0] & 0x01)
					{
						//Sub-image has transparency
						transColorIdx = 0xFF & ext.Bytes[3];
						if (transColorIdx >= colormap->ColorCount)
							transColorIdx = -1;
					}

					break;
				}
			}
		}

		int nComps = 3;
		utils::ImageType type = utils::ImageType::RGB;
		if (transColorIdx >= 0)
		{
			nComps = 4;
			type = utils::ImageType::RGBA;
		}

		if (!destImg.create(type, w, h))
		{
			DGifCloseFile(ctx);
			s_logger.error("cannot initialize destination image");
			return false;
		}

		unsigned char* pDest = destImg.getData();
		const size_t destStride = destImg.getStride();

		//Paint background before sub-image
		if (pBackColor)
		{
			//If there is no specific background color, it is useless to change
			//background color around sub-image as destImg data are already set
			//to 0 (fully transparent black pixel) during creation.
			for (int y = 0; y < subTop; ++y)
			{
				unsigned char* p = pDest;
				for (int x = 0; x < w; ++x)
				{
					p[0] = pBackColor->Red;
					p[1] = pBackColor->Green;
					p[2] = pBackColor->Blue;
					p += nComps;
				}

				pDest += destStride;
			}
		}
		else if (subTop > 0)
			pDest += subTop * destStride;

		//Paint sub-image
		const unsigned char* pSrc = subImg.RasterBits;
		assert(pSrc);

		auto paintLine = [&](int y)
		{
			unsigned char* p = pDest + y * destStride;

			//Paint left of sub-image
			if (pBackColor)
			{
				for (int x = 0; x < subLeft; ++x)
				{
					p[0] = pBackColor->Red;
					p[1] = pBackColor->Green;
					p[2] = pBackColor->Blue;
					p += nComps;
				}
			}
			else if (subLeft > 0)
				p += nComps * subLeft;

			//Paint sub-image
			for (int x = 0; x < subWidth; ++x)
			{
				unsigned char colIdx = *pSrc++;
				if (colIdx < colormap->ColorCount)
				{
					GifColorType* pCol = colormap->Colors + colIdx;
					p[0] = pCol->Red;
					p[1] = pCol->Green;
					p[2] = pCol->Blue;

					if ((nComps == 4) && (colIdx != transColorIdx))
						p[3] = 0xFF;
				}

				p += nComps;
			}

			//Paint right of sub-image
			if (pBackColor)
			{
				for (int x = subLeft + subWidth; x < w; ++x)
				{
					p[0] = pBackColor->Red;
					p[1] = pBackColor->Green;
					p[2] = pBackColor->Blue;
					p += nComps;
				}
			}
		};

		if (subImg.ImageDesc.Interlace)
		{
			static const int interlacedOffsets[] = {0, 4, 2, 1};
			static const int interlacedJumps[] = {8, 8, 4, 2};

			for (int i = 0; i < 4; ++i)
			{
				for (int y = interlacedOffsets[i]; y < subHeight; y += interlacedJumps[i])
					paintLine(y);
			}
		}
		else
		{
			for (int y = 0; y < subHeight; ++y)
				paintLine(y);
		}

		pDest += subHeight * destStride;

		//Paint background after sub-image
		if (pBackColor)
		{
			for (int y = subTop + subHeight; y < h; ++y)
			{
				unsigned char* p = pDest;
				for (int x = 0; x < w; ++x)
				{
					p[0] = pBackColor->Red;
					p[1] = pBackColor->Green;
					p[2] = pBackColor->Blue;
					p += nComps;
				}

				pDest += destStride;
			}
		}

		DGifCloseFile(ctx);
		return true;
	}

	class GIFWriter final
	{
	public:
		GIFWriter(unsigned char*& destData, size_t& destSize);
		GIFWriter(std::FILE* destFile);

		bool isValid() const
		{
			return (m_destFile || m_pDestData);
		}

		bool fitToSize();
		void cleanupOnError();

		static int writeFunc(GifFileType* ctx, const unsigned char* data, int size);

	private:
		GIFWriter(const GIFWriter&) = delete;
		GIFWriter& operator=(const GIFWriter&) = delete;

		int write(const unsigned char* data, int size);

		unsigned char** m_pDestData = nullptr;
		size_t* m_pDestSize = nullptr;
		size_t m_capacity = 0;

		std::FILE* m_destFile = nullptr;
	};

	GIFWriter::GIFWriter(unsigned char*& destData, size_t& destSize)
	{
		if (!destData && !destSize)
		{
			m_pDestData = &destData;
			m_pDestSize = &destSize;
		}
	}

	GIFWriter::GIFWriter(std::FILE* destFile)
	{
		if (destFile)
			m_destFile = destFile;
	}

	bool GIFWriter::fitToSize()
	{
		if (m_pDestData)
		{
			assert(m_pDestSize);
			if (!*m_pDestSize)
			{
				if (*m_pDestData)
				{
					std::free(*m_pDestData);
					*m_pDestData = nullptr;
				}

				m_capacity = 0;
				s_logger.error("buffer write error, buffer is empty");
				return false;
			}

			assert(*m_pDestData);
			if (*m_pDestSize == m_capacity)
				return true;

			*m_pDestData = reinterpret_cast<unsigned char*>(std::realloc(*m_pDestData, *m_pDestSize));
			if (!*m_pDestData)
			{
				*m_pDestSize = 0;
				m_capacity = 0;
				s_logger.error("buffer write error, out of memory");
				return false;
			}

			m_capacity = *m_pDestSize;
			return true;
		}
		else if (m_destFile)
			return true;
		else
		{
			s_logger.error("write error, invalid writer state");
			return false;
		}
	}

	void GIFWriter::cleanupOnError()
	{
		if (m_pDestData)
		{
			assert(m_pDestSize);
			if (*m_pDestData)
			{
				std::free(*m_pDestData);
				*m_pDestData = nullptr;
			}

			*m_pDestSize = 0;
			m_capacity = 0;
		}
	}

	int GIFWriter::writeFunc(GifFileType* ctx, const unsigned char* data, int size)
	{
		if (size > 0)
		{
			assert(ctx);
			GIFWriter* pWriter = reinterpret_cast<GIFWriter*>(ctx->UserData);
			assert(pWriter);
			return pWriter->write(data, size);
		}
		else
			return 0;
	}

	int GIFWriter::write(const unsigned char* data, int size)
	{
		assert(data);
		assert(size > 0);

		if (m_destFile)
			return std::fwrite(data, 1, size, m_destFile);
		else if (m_pDestData)
		{
			assert(m_pDestSize);
			size_t newCapacity = m_capacity;
			if (!newCapacity)
				newCapacity = 8192;

			const size_t newSize = *m_pDestSize + size;
			while (newCapacity < newSize)
				newCapacity <<= 1;

			if (!*m_pDestData)
				*m_pDestData = reinterpret_cast<unsigned char*>(std::malloc(newCapacity));
			else if (newCapacity != m_capacity)
				*m_pDestData = reinterpret_cast<unsigned char*>(std::realloc(*m_pDestData, newCapacity));

			if (!*m_pDestData)
			{
				*m_pDestSize = 0;
				m_capacity = 0;
				s_logger.error("buffer write error, out of memory");
				return 0;
			}
			else
			{
				std::memcpy(*m_pDestData + *m_pDestSize, data, size);
				*m_pDestSize = newSize;
				m_capacity = newCapacity;
				return size;
			}
		}
		else
		{
			s_logger.error("write error, invalid writer state");
			return 0;
		}
	}

	bool writeGIF(const utils::Image& srcImg, GIFWriter& writer)
	{
		if (!srcImg.getData() || !writer.isValid())
			return false;

		//Re-order source image color components for palette quantization
		unsigned int w = srcImg.getWidth();
		unsigned int h = srcImg.getHeight();
		unsigned int srcSize = w * h;
		unsigned char* orderedSrcData = new(std::nothrow) unsigned char[srcSize << 2];
		if (!orderedSrcData)
		{
			s_logger.error("cannot re-order color components for palette quantization, out of memory");
			return false;
		}

		unsigned char* pSrcRed = orderedSrcData;
		unsigned char* pSrcGreen = pSrcRed + srcSize;
		unsigned char* pSrcBlue = pSrcGreen + srcSize;

		int nComps = 1;
		bool hasAlpha = false;
		switch (srcImg.getType())
		{
		case utils::ImageType::LUMINANCE:
			break;

		case utils::ImageType::LUMINANCE_ALPHA:
			nComps = 2;
			hasAlpha = true;
			break;

		case utils::ImageType::RGB:
			nComps = 3;
			break;

		case utils::ImageType::RGBA:
			nComps = 4;
			hasAlpha = true;
			break;
		}

		const unsigned char* pSrc = srcImg.getData();
		const size_t srcStride = srcImg.getStride();
		for (unsigned int y = 0; y < h; ++y)
		{
			const unsigned char* p = pSrc;
			for (unsigned int x = 0; x < w; ++x)
			{
				if (nComps < 3)
				{
					unsigned char col = p[0];
					*pSrcRed++ = col;
					*pSrcGreen++ = col;
					*pSrcBlue++ = col;
				}
				else
				{
					*pSrcRed++ = p[0];
					*pSrcGreen++ = p[1];
					*pSrcBlue++ = p[2];
				}

				p += nComps;
			}

			pSrc += srcStride;
		}

		//Compute quantized palette
		pSrcRed = orderedSrcData;
		pSrcGreen = pSrcRed + srcSize;
		pSrcBlue = pSrcGreen + srcSize;
		unsigned char* pSrcIdx = pSrcBlue + srcSize;

		int colormapSize = hasAlpha ? 255 : 256;
		GifColorType palette[256] = {};
		if (QuantizeBuffer(w, h, &colormapSize, pSrcRed, pSrcGreen, pSrcBlue, pSrcIdx, palette) == GIF_ERROR)
		{
			delete[] orderedSrcData;
			s_logger.error("cannot create quantized palette, out of memory");
			return false;
		}

		unsigned char transparentColorIdx = 0;
		if (hasAlpha)
		{
			//Add a palette entry for transparent ink and set transparent
			//pixels index according to source image alpha value
			assert(colormapSize <= 255);
			transparentColorIdx = colormapSize++;

			pSrc = srcImg.getData();
			for (unsigned int y = 0; y < h; ++y)
			{
				const unsigned char* p = pSrc + nComps - 1;
				for (unsigned int x = 0; x < w; ++x)
				{
					if (*p < TRANSPARENCY_ALPHA_TRESHOLD)
						*pSrcIdx = transparentColorIdx;

					p += nComps;
					pSrcIdx++;
				}

				pSrc += srcStride;
			}

			pSrcIdx = pSrcBlue + srcSize;
		}

		//Write GIF output
		GifFileType* ctx = EGifOpen(&writer, &GIFWriter::writeFunc);
		if (!ctx)
		{
			delete[] orderedSrcData;
			logLastGIFError();
			return false;
		}

		//Write GIF header, logical screen descriptor and global colormap
		ColorMapObject colormap = {};
		colormap.BitsPerPixel = BitSize(colormapSize);
		colormap.ColorCount = 1 << colormap.BitsPerPixel;
		assert(colormap.ColorCount <= 256);
		colormap.Colors = palette;

		if (hasAlpha)
			EGifSetGifVersion("89a");
		else
			EGifSetGifVersion("87a");

		if (EGifPutScreenDesc(ctx, w, h, colormap.BitsPerPixel, transparentColorIdx, &colormap) == GIF_ERROR)
		{
			logLastGIFError();
			EGifCloseFile(ctx);
			delete[] orderedSrcData;
			writer.cleanupOnError();
			return false;
		}

		if (hasAlpha)
		{
			//Write "Graphics Control Extension" block to enable transparency
			const unsigned char extBuffer[] = {0x01, 0x00, 0x00, transparentColorIdx};
			if (EGifPutExtension(ctx, GRAPHICS_EXT_FUNC_CODE, 4, extBuffer) == GIF_ERROR)
			{
				logLastGIFError();
				EGifCloseFile(ctx);
				delete[] orderedSrcData;
				writer.cleanupOnError();
				return false;
			}
		}

		//Write image descriptor
		if (EGifPutImageDesc(ctx, 0, 0, w, h, 0, nullptr) == GIF_ERROR)
		{
			logLastGIFError();
			EGifCloseFile(ctx);
			delete[] orderedSrcData;
			writer.cleanupOnError();
			return false;
		}

		//Write image data
		for (unsigned int y = 0; y < h; ++y)
		{
			if (EGifPutLine(ctx, pSrcIdx, w) == GIF_ERROR)
			{
				logLastGIFError();
				EGifCloseFile(ctx);
				delete[] orderedSrcData;
				writer.cleanupOnError();
				return false;
			}

			pSrcIdx += w;
		}

		EGifCloseFile(ctx);
		delete[] orderedSrcData;
		return writer.fitToSize();
	}
}

namespace utils
{
	namespace internal
	{
		bool ImageIO_gif::decodeImageData(const unsigned char* srcData, size_t srcSize, Image& destImg)
		{
			GIFReader reader(srcData, srcSize);
			return readGIF(reader, destImg);
		}

		bool ImageIO_gif::encodeImageData(const Image& srcImg, unsigned char*& destData, size_t& destSize)
		{
			GIFWriter writer(destData, destSize);
			return writeGIF(srcImg, writer);
		}

		bool ImageIO_gif::decodeImageFile(std::FILE* srcFile, Image& destImg)
		{
			GIFReader reader(srcFile);
			return readGIF(reader, destImg);
		}

		bool ImageIO_gif::encodeImageFile(const Image& srcImg, std::FILE* destFile)
		{
			GIFWriter writer(destFile);
			return writeGIF(srcImg, writer);
		}
	}
}
