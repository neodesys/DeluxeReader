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

#ifndef _LOGGER_H_
#define	_LOGGER_H_

#include <cstdarg>
#include <atomic>

namespace utils
{
	enum struct LogLevel : unsigned char
	{
		INFO,
		WARNING,
		ERROR
	};

	class Logger;
	class ILogFormatter
	{
	public:
		virtual ~ILogFormatter() = default;

		//Implementation MUST be thread-safe
		virtual void logOut(const Logger& logger, LogLevel level, const char* format, std::va_list varArgs) = 0;
	};

	class Logger final
	{
	public:
		//WARNING: moduleName string content is not copied. The pointer is
		//directly stored internally, so moduleName memory buffer must have a
		//lifetime superior to the Logger instance. Normally, moduleName
		//should be a string literal set at compile-time. Common usage of
		//Logger class is as follows:
		//class MyClass
		//{
		//protected:
		//    static const Logger s_logger;
		//};
		//
		//const Logger MyClass::s_logger("MyClass");
		Logger(const char* moduleName)
		{
			if (moduleName)
				m_moduleName = moduleName;
		}

		//Thread-safe and returned value is always different from nullptr
		const char* getModuleName() const
		{
			return m_moduleName;
		}

		//Thread-safe
		void log(LogLevel level, const char* format, ...) const;

		//Thread-safe
		void info(const char* format, ...) const;

		//Thread-safe
		void warning(const char* format, ...) const;

		//Thread-safe
		void error(const char* format, ...) const;

		//Thread-safe
		static void setLogFormatter(ILogFormatter* pLogFormatter);

	private:
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		const char* m_moduleName = "";

		static std::atomic<ILogFormatter*> s_logFormatter;
	};
}

#endif //_LOGGER_H_
