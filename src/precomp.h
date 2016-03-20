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

#ifndef _PRECOMP_H_
#define _PRECOMP_H_

#ifdef __GNUC__ //GNU-compatible compilers: g++, mingw and clang
#if __cplusplus < 201103L
#error You must use a C++11 compatible compiler
#endif //__cplusplus < 201103L
#elif defined(_MSC_VER) //Visual C++ compiler
#if _MSC_VER < 1900
#error You must use a C++11 compatible compiler
#endif //_MSC_VER < 1900
#else
#error Unsupported compiler: currently supported compilers are\
       gnu-compatible ones (gcc, mingw and clang) and Visual C++
#endif

#include <cstddef>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <ctime>
#include <csetjmp>
#include <cassert>

#include <atomic>
#include <new>

#include <sys/stat.h>

#include <gif_lib.h>
#include <jpeglib.h>
#include <jerror.h>
#define PNG_SKIP_SETJMP_CHECK
#include <png.h>
#include <tiffio.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <mupdf/fitz.h>
#ifdef __cplusplus
}
#endif

#include <archive.h>
#include <archive_entry.h>

#ifdef __linux__
#include <csignal>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <GL/glx.h>
#endif //__linux__

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <io.h>
#endif //_WIN32

#endif //_PRECOMP_H_
