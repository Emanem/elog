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
#include <thread>
#include <list>

void __attribute__ ((noinline)) compile_log_optimized(void) {
	ELOG_INFO(123, 456.789); 
}

int main(int argc, const char *argv[]) {
	// some static asserts to ensure we have
	// control over structure sizes...
	static_assert(sizeof(elog::entry)==512, "Size of elog::entry is not 512 B");

	ELOG_INIT("test.log");
	ELOG_DEBUG("This ", "is ", "a ", 123, " test!");
	// add threads to print the log
	std::list<std::thread>	th_list;
	for(int i = 0; i < 4; ++i) {
		th_list.push_back(std::thread([]() -> void {
			for(size_t j = 0; j < 1024*1024; ++j) {
				ELOG_INFO(__FILE__, ':', __LINE__, " a counter number: ", j);
				ELOG_FATAL("This is a test!");
				//if(j > 512*1024) ELOG_SET_LEVEL(elog::level_fatal);
			}
		}));
	}
	for(auto& i : th_list) i.join();
	ELOG_CLEANUP();
}

