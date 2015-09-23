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

#ifndef _FILELOGGER_H_
#define	_FILELOGGER_H_

#include <cstdio>

#include "Logger.h"

namespace utils
{
	class FileLogger final : public ILogFormatter
	{
	public:
		//WARNING: construction/destruction is not thread-safe.
		//Common usage is to create the FileLogger instance at program start
		//before any thread using logging facilities, and to destroy the
		//instance at program end after that any thread using it has terminated.
		FileLogger(const char* fileName, bool bAppend = false);
		virtual ~FileLogger() override final;

		//Thread-safe
		void setLogLevel(LogLevel logLevel);

		//Thread-safe
		LogLevel getLogLevel() const;

		//Thread-safe
		virtual void logOut(const Logger& logger, LogLevel level, const char* format, std::va_list varArgs) override final;

		//Thread-safe
		void flushLog();

	private:
		FileLogger(const FileLogger&) = delete;
		FileLogger& operator=(const FileLogger&) = delete;

		std::FILE* m_logFile = nullptr;
		std::atomic<LogLevel> m_logLevel = {LogLevel::ERROR};
	};
}

#endif //_FILELOGGER_H_
