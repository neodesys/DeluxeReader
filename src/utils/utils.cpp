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

#include "utils.h"

#include <cctype>
#include <cassert>
#include <cstdlib>

namespace utils
{
	int strnatcmp(const char* s1, const char* s2)
	{
		//TODO: Test strnatcmp

		assert(s1);
		assert(s2);

		for (;;)
		{
			//Skip spaces
			while (std::isspace(*s1))
				++s1;

			while (std::isspace(*s2))
				++s2;

			//Test digits
			if (std::isdigit(*s1) && std::isdigit(*s2))
			{
				double a = std::strtod(s1, const_cast<char**>(&s1));
				double b = std::strtod(s2, const_cast<char**>(&s2));

				if (a < b)
					return -1;
				else if (a > b)
					return 1;
			}

			//Test other characters
			if (*s1 < *s2)
				return -1;
			else if (*s1 > *s2)
				return 1;
			else if (*s1 == '\0')
				return 0;

			++s1;
			++s2;
		}
	}
}
