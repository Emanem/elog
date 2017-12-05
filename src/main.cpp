/*
*	elog (C) 2017 E. Oriani, ema <AT> fastwebnet <DOT> it
*
*	This file is part of elog.
*
*	elog is free software: you can redistribute it and/or modify
*	it under the terms of the GNU Lesser General Public License as published by
*	the Free Software Foundation, either version 3 of the License, or
*	(at your option) any later version.
*
*	elog is distributed in the hope that it will be useful,
*	but WITHOUT ANY WARRANTY; without even the implied warranty of
*	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*	GNU General Public License for more details.
*
*	You should have received a copy of the GNU General Public License
*	along with elog.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "elog.h"

int main(int argc, const char *argv[]) {
	// some static asserts to ensure we have
	// control over structure sizes...
	static_assert(sizeof(elog::entry)==512, "Size of elog::entry is not 512 B");

	ELOG_INIT("test.log");
	ELOG_DEBUG("This ", "is ", "a ", 123, " test!");
	for(size_t i = 0; i < 1024*1024; ++i) {
		ELOG_INFO(__FILE__, ':', __LINE__, " a counter number: ", i);
		ELOG_FATAL("This is a test!");
		if(i > 512*1024) ELOG_SET_LEVEL(elog::level_fatal);
	}
	ELOG_CLEANUP();
}

