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

#include "FileLogger.h"

#include <ctime>

namespace
{
	//Size of a string buffer (including terminating \0 character) for ISO-8601
	//representation of UTC dates using the extended format.
	//ex: 2015-04-07T11:35:15Z
	const size_t DATE_BUFFER_SIZE = 21;
}

namespace utils
{
	FileLogger::FileLogger(const char* fileName, bool bAppend)
	{
		if (fileName && (fileName[0] != '\0'))
		{
			if (bAppend)
				m_logFile = std::fopen(fileName, "at");
			else
				m_logFile = std::fopen(fileName, "wt");
		}
	}

	FileLogger::~FileLogger()
	{
		if (m_logFile)
		{
			std::fclose(m_logFile);
			m_logFile = nullptr;
		}
	}

	void FileLogger::setLogLevel(LogLevel logLevel)
	{
		m_logLevel.store(logLevel, std::memory_order_release);
	}

	LogLevel FileLogger::getLogLevel() const
	{
		return m_logLevel.load(std::memory_order_consume);
	}

	void FileLogger::logOut(const Logger& logger, LogLevel level, const char* format, std::va_list varArgs)
	{
		if (m_logFile)
		{
			LogLevel minLevel = m_logLevel.load(std::memory_order_consume);
			if (level >= minLevel)
			{
				char dateBuffer[DATE_BUFFER_SIZE] = "";
				std::time_t timestamp = std::time(nullptr);
				if (timestamp != -1)
				{
					std::tm timeStruct = {};
					if (gmtime_r(&timestamp, &timeStruct))
					{
						if (std::strftime(dateBuffer, DATE_BUFFER_SIZE, "%Y-%m-%dT%H:%M:%SZ", &timeStruct) != DATE_BUFFER_SIZE - 1)
							dateBuffer[0] = '\0';
					}
				}

				flockfile(m_logFile);
				fputc_unlocked('[', m_logFile);
				fputs_unlocked(dateBuffer, m_logFile);
				fputs_unlocked("][", m_logFile);
				fputs_unlocked(logger.getModuleName(), m_logFile);
				fputs_unlocked("][", m_logFile);
				switch (level)
				{
				case LogLevel::INFO:
					fputs_unlocked("INFO", m_logFile);
					break;

				case LogLevel::WARNING:
					fputs_unlocked("WARNING", m_logFile);
					break;

				case LogLevel::ERROR:
					fputs_unlocked("ERROR", m_logFile);
					break;
				}
				fputc_unlocked(']', m_logFile);

				if (format && (format[0] != '\0'))
				{
					fputs_unlocked(": ", m_logFile);
					std::vfprintf(m_logFile, format, varArgs);
				}

				fputc_unlocked('\n', m_logFile);
				funlockfile(m_logFile);
			}
		}
	}

	void FileLogger::flushLog()
	{
		if (m_logFile)
			std::fflush(m_logFile);
	}
}
