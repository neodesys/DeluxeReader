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

#ifndef _UTILS_H_
#define	_UTILS_H_

namespace utils
{
	//Equivalent of strcmp but using natural sort ordering
	//@see https://en.wikipedia.org/wiki/Natural_sort_order
	//WARNING: s1 and s2 strings must be valid strings (different from nullptr)
	int strnatcmp(const char* s1, const char* s2);
}

#endif //_UTILS_H_
