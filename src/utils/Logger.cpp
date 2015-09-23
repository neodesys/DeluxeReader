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

#include "Logger.h"

namespace utils
{
	void Logger::log(LogLevel level, const char* format, ...) const
	{
		ILogFormatter* p = s_logFormatter.load(std::memory_order_consume);
		if (p)
		{
			std::va_list varArgs;
			va_start(varArgs, format);
			p->logOut(*this, level, format, varArgs);
			va_end(varArgs);
		}
	}

	void Logger::info(const char* format, ...) const
	{
		ILogFormatter* p = s_logFormatter.load(std::memory_order_consume);
		if (p)
		{
			std::va_list varArgs;
			va_start(varArgs, format);
			p->logOut(*this, LogLevel::INFO, format, varArgs);
			va_end(varArgs);
		}
	}

	void Logger::warning(const char* format, ...) const
	{
		ILogFormatter* p = s_logFormatter.load(std::memory_order_consume);
		if (p)
		{
			std::va_list varArgs;
			va_start(varArgs, format);
			p->logOut(*this, LogLevel::WARNING, format, varArgs);
			va_end(varArgs);
		}
	}

	void Logger::error(const char* format, ...) const
	{
		ILogFormatter* p = s_logFormatter.load(std::memory_order_consume);
		if (p)
		{
			std::va_list varArgs;
			va_start(varArgs, format);
			p->logOut(*this, LogLevel::ERROR, format, varArgs);
			va_end(varArgs);
		}
	}

	void Logger::setLogFormatter(ILogFormatter* pLogFormatter)
	{
		s_logFormatter.store(pLogFormatter, std::memory_order_release);
	}

	std::atomic<ILogFormatter*> Logger::s_logFormatter(nullptr);
}
