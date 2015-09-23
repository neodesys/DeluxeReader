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

#include "ImageIO_png.h"

#include <cstdlib>
#include <cstring>
#include <csetjmp>
#include <cassert>

#define PNG_SKIP_SETJMP_CHECK
#include <png.h>

#include "../Image.h"

#if PNG_LIBPNG_VER < 10250
#error You must link with libpng version 1.2.50 or newer
#endif //PNG_LIBPNG_VER < 10250

namespace
{
	const size_t PNG_SIGNATURE_LENGTH = 8;

	const utils::Logger s_logger("utils::Image::Codec_PNG");

	class PNGError final
	{
	public:
		PNGError() = default;

		std::jmp_buf& getJumpData()
		{
			return m_jumpBuffer;
		}

		static void error_fn(png_structp ctx, const char* msg);
		static void warning_fn(png_structp ctx, const char* msg);

	private:
		PNGError(const PNGError&) = delete;
		PNGError& operator=(const PNGError&) = delete;

		std::jmp_buf m_jumpBuffer = {};
	};

	void PNGError::error_fn(png_structp ctx, const char* msg)
	{
		assert(ctx);
		PNGError* pError = reinterpret_cast<PNGError*>(ctx->error_ptr);
		assert(pError);

		if (msg)
			s_logger.error(msg);

		std::longjmp(pError->m_jumpBuffer, 1);
	}

	void PNGError::warning_fn(png_structp, const char* msg)
	{
		if (msg)
			s_logger.warning(msg);
	}

	class PNGReader final
	{
	public:
		PNGReader(const unsigned char* srcData, size_t srcSize);
		PNGReader(std::FILE* srcFile);

		bool isValid() const
		{
			return (m_srcFile || m_srcData);
		}

		bool verifySignature();

		static void read_data_fn(png_structp ctx, unsigned char* data, size_t size);

	private:
		PNGReader(const PNGReader&) = delete;
		PNGReader& operator=(const PNGReader&) = delete;

		void read(png_structp ctx, unsigned char* data, size_t size);

		const unsigned char* m_srcData = nullptr;
		size_t m_srcSize = 0;
		size_t m_pos = 0;

		std::FILE* m_srcFile = nullptr;
	};

	PNGReader::PNGReader(const unsigned char* srcData, size_t srcSize)
	{
		if (srcData && srcSize)
		{
			m_srcData = srcData;
			m_srcSize = srcSize;
		}
	}

	PNGReader::PNGReader(std::FILE* srcFile)
	{
		if (srcFile)
			m_srcFile = srcFile;
	}

	bool PNGReader::verifySignature()
	{
		if (m_srcFile)
		{
			unsigned char signature[PNG_SIGNATURE_LENGTH] = {};
			if ((std::fread(signature, 1, PNG_SIGNATURE_LENGTH, m_srcFile) == PNG_SIGNATURE_LENGTH) && !png_sig_cmp(signature, 0, PNG_SIGNATURE_LENGTH))
				return true;
		}
		else if (m_srcData)
		{
			assert(m_srcSize);
			if (m_pos + PNG_SIGNATURE_LENGTH <= m_srcSize)
			{
				const unsigned char* p = m_srcData + m_pos;
				m_pos += PNG_SIGNATURE_LENGTH;

				if (!png_sig_cmp(const_cast<png_bytep>(p), 0, PNG_SIGNATURE_LENGTH))
					return true;
			}
		}

		return false;
	}

	void PNGReader::read_data_fn(png_structp ctx, unsigned char* data, size_t size)
	{
		if (size)
		{
			assert(ctx);
			PNGReader* pReader = reinterpret_cast<PNGReader*>(ctx->io_ptr);
			assert(pReader);
			pReader->read(ctx, data, size);
		}
	}

	void PNGReader::read(png_structp ctx, unsigned char* data, size_t size)
	{
		assert(data);
		assert(size);

		if (m_srcFile)
		{
			if (std::fread(data, 1, size, m_srcFile) != size)
				png_error(ctx, "read error, end of file reached");
		}
		else if (m_srcData)
		{
			assert(m_srcSize);
			if (m_pos + size <= m_srcSize)
			{
				std::memcpy(data, m_srcData + m_pos, size);
				m_pos += size;
			}
			else
				png_error(ctx, "read error, end of buffer reached");
		}
		else
			png_error(ctx, "read error, invalid reader state");
	}

	bool readPNG(PNGReader& reader, utils::Image& destImg)
	{
		if (destImg.getData() || !reader.isValid())
			return false;

		if (!reader.verifySignature())
		{
			s_logger.error("data are not encoded in PNG format (invalid signature)");
			return false;
		}

		PNGError error;
		png_structp ctx = png_create_read_struct(PNG_LIBPNG_VER_STRING, &error, &PNGError::error_fn, &PNGError::warning_fn);
		if (!ctx)
		{
			s_logger.error("cannot initialize PNG decoder");
			return false;
		}

		png_infop ctxInfo = png_create_info_struct(ctx);
		if (!ctxInfo)
		{
			png_destroy_read_struct(&ctx, nullptr, nullptr);
			s_logger.error("cannot initialize PNG decoder");
			return false;
		}

		if (setjmp(error.getJumpData()))
		{
			png_destroy_read_struct(&ctx, &ctxInfo, nullptr);
			destImg.destroy();
			return false;
		}

		png_set_read_fn(ctx, &reader, &PNGReader::read_data_fn);
		png_set_sig_bytes(ctx, PNG_SIGNATURE_LENGTH);

		png_read_info(ctx, ctxInfo);
		png_set_strip_16(ctx);
		png_set_packing(ctx);

		if ((ctx->bit_depth < 8) ||
			(ctx->color_type == PNG_COLOR_TYPE_PALETTE) ||
			png_get_valid(ctx, ctxInfo, PNG_INFO_tRNS))
			png_set_expand(ctx);

		png_read_update_info(ctx, ctxInfo);
		utils::ImageType type = utils::ImageType::LUMINANCE;
		switch (ctxInfo->channels)
		{
		case 1:
			break;

		case 2:
			type = utils::ImageType::LUMINANCE_ALPHA;
			break;

		case 3:
			type = utils::ImageType::RGB;
			break;

		case 4:
			type = utils::ImageType::RGBA;
			break;

		default:
			png_destroy_read_struct(&ctx, &ctxInfo, nullptr);
			s_logger.error("unknown channels in source image");
			return false;
		}

		if (!destImg.create(type, ctxInfo->width, ctxInfo->height))
		{
			png_destroy_read_struct(&ctx, &ctxInfo, nullptr);
			s_logger.error("cannot initialize destination image");
			return false;
		}

		unsigned char* pDest = destImg.getData();
		const size_t destStride = destImg.getStride();
		assert(ctxInfo->rowbytes == destStride);

		int nPasses = png_set_interlace_handling(ctx);
		for (int i = 0; i < nPasses; ++i)
		{
			unsigned char* p = pDest;
			for (png_uint_32 y = 0; y < ctxInfo->height; ++y)
			{
				png_read_row(ctx, p, nullptr);
				p += destStride;
			}
		}

		png_read_end(ctx, ctxInfo);
		png_destroy_read_struct(&ctx, &ctxInfo, nullptr);
		return true;
	}

	class PNGWriter final
	{
	public:
		PNGWriter(unsigned char*& destData, size_t& destSize);
		PNGWriter(std::FILE* destFile);

		bool isValid() const
		{
			return (m_destFile || m_pDestData);
		}

		bool fitToSize();
		void cleanupOnError();

		static void write_data_fn(png_structp ctx, unsigned char* data, size_t size);
		static void output_flush_fn(png_structp ctx);

	private:
		PNGWriter(const PNGWriter&) = delete;
		PNGWriter& operator=(const PNGWriter&) = delete;

		void write(png_structp ctx, unsigned char* data, size_t size);
		void flush();

		unsigned char** m_pDestData = nullptr;
		size_t* m_pDestSize = nullptr;
		size_t m_capacity = 0;

		std::FILE* m_destFile = nullptr;
	};

	PNGWriter::PNGWriter(unsigned char*& destData, size_t& destSize)
	{
		if (!destData && !destSize)
		{
			m_pDestData = &destData;
			m_pDestSize = &destSize;
		}
	}

	PNGWriter::PNGWriter(std::FILE* destFile)
	{
		if (destFile)
			m_destFile = destFile;
	}

	bool PNGWriter::fitToSize()
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

	void PNGWriter::cleanupOnError()
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

	void PNGWriter::write_data_fn(png_structp ctx, unsigned char* data, size_t size)
	{
		if (size)
		{
			assert(ctx);
			PNGWriter* pWriter = reinterpret_cast<PNGWriter*>(ctx->io_ptr);
			assert(pWriter);
			pWriter->write(ctx, data, size);
		}
	}

	void PNGWriter::output_flush_fn(png_structp ctx)
	{
		assert(ctx);
		PNGWriter* pWriter = reinterpret_cast<PNGWriter*>(ctx->io_ptr);
		assert(pWriter);
		pWriter->flush();
	}

	void PNGWriter::write(png_structp ctx, unsigned char* data, size_t size)
	{
		assert(data);
		assert(size);

		if (m_destFile)
		{
			if (std::fwrite(data, 1, size, m_destFile) != size)
				png_error(ctx, "file write error");
		}
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
				png_error(ctx, "buffer write error, out of memory");
			}
			else
			{
				std::memcpy(*m_pDestData + *m_pDestSize, data, size);
				*m_pDestSize = newSize;
				m_capacity = newCapacity;
			}
		}
		else
			png_error(ctx, "write error, invalid writer state");
	}

	void PNGWriter::flush()
	{
		if (m_destFile)
			std::fflush(m_destFile);
	}

	bool writePNG(const utils::Image& srcImg, PNGWriter& writer)
	{
		if (!srcImg.getData() || !writer.isValid())
			return false;

		PNGError error;
		png_structp ctx = png_create_write_struct(PNG_LIBPNG_VER_STRING, &error, &PNGError::error_fn, &PNGError::warning_fn);
		if (!ctx)
		{
			s_logger.error("cannot initialize PNG encoder");
			return false;
		}

		png_infop ctxInfo = png_create_info_struct(ctx);
		if (!ctxInfo)
		{
			png_destroy_write_struct(&ctx, nullptr);
			s_logger.error("cannot initialize PNG encoder");
			return false;
		}

		if (setjmp(error.getJumpData()))
		{
			png_destroy_write_struct(&ctx, &ctxInfo);
			writer.cleanupOnError();
			return false;
		}

		png_set_write_fn(ctx, &writer, &PNGWriter::write_data_fn, &PNGWriter::output_flush_fn);

		int colorType = PNG_COLOR_TYPE_GRAY;
		switch (srcImg.getType())
		{
		case utils::ImageType::LUMINANCE:
			break;

		case utils::ImageType::LUMINANCE_ALPHA:
			colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
			break;

		case utils::ImageType::RGB:
			colorType = PNG_COLOR_TYPE_RGB;
			break;

		case utils::ImageType::RGBA:
			colorType = PNG_COLOR_TYPE_RGB_ALPHA;
			break;
		}
		png_set_IHDR(ctx, ctxInfo, srcImg.getWidth(), srcImg.getHeight(), 8, colorType, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
		png_set_compression_level(ctx, Z_BEST_COMPRESSION);
		png_write_info(ctx, ctxInfo);

		const unsigned char* pSrc = srcImg.getData();
		const size_t srcStride = srcImg.getStride();

		int nPasses = png_set_interlace_handling(ctx);
		for (int i = 0; i < nPasses; ++i)
		{
			const unsigned char* p = pSrc;
			for (png_uint_32 y = 0; y < ctx->height; ++y)
			{
				png_write_row(ctx, const_cast<png_bytep>(p));
				p += srcStride;
			}
		}

		png_write_end(ctx, ctxInfo);
		png_destroy_write_struct(&ctx, &ctxInfo);
		return writer.fitToSize();
	}
}

namespace utils
{
	namespace internal
	{
		bool ImageIO_png::decodeImageData(const unsigned char* srcData, size_t srcSize, Image& destImg)
		{
			PNGReader reader(srcData, srcSize);
			return readPNG(reader, destImg);
		}

		bool ImageIO_png::encodeImageData(const Image& srcImg, unsigned char*& destData, size_t& destSize)
		{
			PNGWriter writer(destData, destSize);
			return writePNG(srcImg, writer);
		}

		bool ImageIO_png::decodeImageFile(std::FILE* srcFile, Image& destImg)
		{
			PNGReader reader(srcFile);
			return readPNG(reader, destImg);
		}

		bool ImageIO_png::encodeImageFile(const Image& srcImg, std::FILE* destFile)
		{
			PNGWriter writer(destFile);
			return writePNG(srcImg, writer);
		}
	}
}
