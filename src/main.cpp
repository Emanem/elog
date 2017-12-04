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

