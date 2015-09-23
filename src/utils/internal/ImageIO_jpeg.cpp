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

#include "ImageIO_jpeg.h"

#include <cstdlib>
#include <csetjmp>
#include <cassert>
#include <new>

#include <jpeglib.h>
#include <jerror.h>

#include "../Image.h"

#if JPEG_LIB_VERSION < 62
#error You must link with libjpeg version 62 or newer
#endif //JPEG_LIB_VERSION < 62

namespace
{
	const int JPEG_OUTPUT_QUALITY = 100;

	const utils::Logger s_logger("utils::Image::Codec_JPEG");

	class JPEGError final
	{
	public:
		JPEGError();

		jpeg_error_mgr* getErrorHanlder()
		{
			assert(&m_errorHandler == reinterpret_cast<jpeg_error_mgr*>(this));
			return &m_errorHandler;
		}

		std::jmp_buf& getJumpData()
		{
			return m_jumpBuffer;
		}

	private:
		JPEGError(const JPEGError&) = delete;
		JPEGError& operator=(const JPEGError&) = delete;

		jpeg_error_mgr m_errorHandler = {};
		std::jmp_buf m_jumpBuffer = {};

		static void error_exit(j_common_ptr cinfo);
		static void output_message(j_common_ptr cinfo);
	};

	JPEGError::JPEGError()
	{
		jpeg_std_error(&m_errorHandler);
		m_errorHandler.error_exit = &JPEGError::error_exit;
		m_errorHandler.output_message = &JPEGError::output_message;
	}

	void JPEGError::error_exit(j_common_ptr cinfo)
	{
		assert(cinfo);
		char errorBuffer[JMSG_LENGTH_MAX] = "";
		cinfo->err->format_message(cinfo, errorBuffer);
		s_logger.error(errorBuffer);

		//cinfo->err points to &m_errorHandler of a JPEGError instance.
		//As m_errorHandler is the first field of the JPEGError class, it can
		//be cast safely to the address of the JPEGError instance.
		JPEGError* pError = reinterpret_cast<JPEGError*>(cinfo->err);
		assert(pError);
		std::longjmp(pError->m_jumpBuffer, 1);
	}

	void JPEGError::output_message(j_common_ptr cinfo)
	{
		assert(cinfo);
		char warningBuffer[JMSG_LENGTH_MAX] = "";
		cinfo->err->format_message(cinfo, warningBuffer);
		s_logger.warning(warningBuffer);
	}

	struct ReadParams
	{
		bool useFile;

		union
		{
			struct
			{
				const unsigned char* srcData;
				size_t srcSize;
			} buffer;

			std::FILE* srcFile;
		} params;
	};

	bool readJPEG(ReadParams& srcParams, utils::Image& destImg)
	{
		if (destImg.getData())
			return false;

		if (srcParams.useFile)
		{
			if (!srcParams.params.srcFile)
				return false;
		}
		else if (!srcParams.params.buffer.srcData || !srcParams.params.buffer.srcSize)
			return false;

		jpeg_decompress_struct ctx = {};

		JPEGError error;
		ctx.err = error.getErrorHanlder();
		if (setjmp(error.getJumpData()))
		{
			jpeg_destroy_decompress(&ctx);
			destImg.destroy();
			return false;
		}

		jpeg_create_decompress(&ctx);
		if (srcParams.useFile)
			jpeg_stdio_src(&ctx, srcParams.params.srcFile);
		else
			jpeg_mem_src(&ctx, const_cast<unsigned char*>(srcParams.params.buffer.srcData), srcParams.params.buffer.srcSize);

		//As reading jpeg data from stdio or from memory cannot be suspended,
		//it is useless to check returned status for suspension errors. All
		//other errors are already processed through the internal error handler.
		jpeg_read_header(&ctx, TRUE);

		utils::ImageType type = utils::ImageType::RGB;
		ctx.out_color_space = JCS_RGB;
		if (ctx.jpeg_color_space == JCS_GRAYSCALE)
		{
			type = utils::ImageType::LUMINANCE;
			ctx.out_color_space = JCS_GRAYSCALE;
		}

		jpeg_start_decompress(&ctx);

		if (!destImg.create(type, ctx.output_width, ctx.output_height))
		{
			jpeg_destroy_decompress(&ctx);
			s_logger.error("cannot initialize destination image");
			return false;
		}

		unsigned char* pDest = destImg.getData();
		const size_t destStride = destImg.getStride();
		while (ctx.output_scanline < ctx.output_height)
		{
			jpeg_read_scanlines(&ctx, &pDest, 1);
			pDest += destStride;
		}

		jpeg_finish_decompress(&ctx);
		jpeg_destroy_decompress(&ctx);
		return true;
	}

	struct WriteParams
	{
		bool useFile;

		union
		{
			struct
			{
				unsigned char*& destData;
				size_t& destSize;
			} buffer;

			std::FILE* destFile;
		} params;
	};

	bool writeJPEG(const utils::Image& srcImg, WriteParams& destParams)
	{
		if (!srcImg.getData())
			return false;

		if (destParams.useFile)
		{
			if (!destParams.params.destFile)
				return false;
		}
		else if (destParams.params.buffer.destData || destParams.params.buffer.destSize)
			return false;

		jpeg_compress_struct ctx = {};
		unsigned char* convBuffer = nullptr;

		JPEGError error;
		ctx.err = error.getErrorHanlder();
		if (setjmp(error.getJumpData()))
		{
			if (convBuffer)
			{
				delete[] convBuffer;
				convBuffer = nullptr;
			}

			jpeg_destroy_compress(&ctx);

			if (!destParams.useFile)
			{
				if (destParams.params.buffer.destData)
				{
					std::free(destParams.params.buffer.destData);
					destParams.params.buffer.destData = nullptr;
				}

				destParams.params.buffer.destSize = 0;
			}

			return false;
		}

		jpeg_create_compress(&ctx);
		if (destParams.useFile)
			jpeg_stdio_dest(&ctx, destParams.params.destFile);
		else
			jpeg_mem_dest(&ctx, &destParams.params.buffer.destData, &destParams.params.buffer.destSize);

		bool stripAlphaAfterLuminance = false;
		ctx.image_width = srcImg.getWidth();
		ctx.image_height = srcImg.getHeight();
		switch (srcImg.getType())
		{
		case utils::ImageType::LUMINANCE:
			ctx.input_components = 1;
			ctx.in_color_space = JCS_GRAYSCALE;
			break;

		case utils::ImageType::LUMINANCE_ALPHA:
			ctx.input_components = 1;
			ctx.in_color_space = JCS_GRAYSCALE;
			stripAlphaAfterLuminance = true;
			break;

		case utils::ImageType::RGB:
			ctx.input_components = 3;
			ctx.in_color_space = JCS_RGB;
			break;

		case utils::ImageType::RGBA:
			ctx.input_components = 4;
			ctx.in_color_space = JCS_EXT_RGBA;
			break;
		}

		jpeg_set_defaults(&ctx);
		jpeg_set_quality(&ctx, JPEG_OUTPUT_QUALITY, TRUE);
		jpeg_start_compress(&ctx, TRUE);

		const unsigned char* pSrc = srcImg.getData();
		const size_t srcStride = srcImg.getStride();
		if (stripAlphaAfterLuminance)
		{
			const size_t halfStride = srcStride >> 1;
			convBuffer = new(std::nothrow) unsigned char[halfStride];
			if (!convBuffer)
				ERREXIT1(&ctx, JERR_OUT_OF_MEMORY, 0);

			while (ctx.next_scanline < ctx.image_height)
			{
				for (size_t x = 0; x < halfStride; ++x)
					convBuffer[x] = pSrc[x << 1];

				jpeg_write_scanlines(&ctx, &convBuffer, 1);
				pSrc += srcStride;
			}

			delete[] convBuffer;
			convBuffer = nullptr;
		}
		else
		{
			while (ctx.next_scanline < ctx.image_height)
			{
				jpeg_write_scanlines(&ctx, const_cast<JSAMPARRAY>(&pSrc), 1);
				pSrc += srcStride;
			}
		}

		jpeg_finish_compress(&ctx);
		jpeg_destroy_compress(&ctx);
		return true;
	}
}

namespace utils
{
	namespace internal
	{
		bool ImageIO_jpeg::decodeImageData(const unsigned char* srcData, size_t srcSize, Image& destImg)
		{
			ReadParams srcParams = {false, {srcData, srcSize}};
			return readJPEG(srcParams, destImg);
		}

		bool ImageIO_jpeg::encodeImageData(const Image& srcImg, unsigned char*& destData, size_t& destSize)
		{
			WriteParams destParams = {false, {destData, destSize}};
			return writeJPEG(srcImg, destParams);
		}

		bool ImageIO_jpeg::decodeImageFile(std::FILE* srcFile, Image& destImg)
		{
			ReadParams srcParams = {true, {}};
			srcParams.params.srcFile = srcFile;
			return readJPEG(srcParams, destImg);
		}

		bool ImageIO_jpeg::encodeImageFile(const Image& srcImg, std::FILE* destFile)
		{
			WriteParams destParams = {true, {}};
			destParams.params.destFile = destFile;
			return writeJPEG(srcImg, destParams);
		}
	}
}
