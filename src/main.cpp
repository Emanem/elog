#include "elog.h"

int main(int argc, const char *argv[]) {
	LOG_INIT("test.log");
	LOG("This ", "is ", "a ", 123, " test!");
	for(size_t i = 0; i < 1024*1024; ++i) {
		LOG(__FILE__, ':', __LINE__, " a counter number: ", i);
	}
	LOG_CLEANUP();
}

