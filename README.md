# elog
A C++11 logging framework with high emphasys on performance.

Please note this is a template library (released under LGPLv3) hence the main itself is simply a test to show the performance/usage; the current version compiles on Linux, still it should be easy to adapt to Windows or other architectures.

Also note this is now pretty much ***work in progress***!

## Building
Simply invoke `make` (`make release` for optimized build) and the main should compile.

## How do I include in my project(s)?
Feel free to copy the file `elog.h`, `elog.cpp` and include it wherever needed; see [license](#license).

## How do I use it in my code?
After including the header and cpp, one can use respective *macros* as defined in `main.cpp`. As example:
``` c++
ELOG_INIT("test.log");
...
...
ELOG_INFO("some ", "log ", "here ", 123, '\t', -12.45);
...
...
ELOG_SET_LEVEL(elog::level_fatal|elog::level_error); // only log errors and fatal events
ELOG_DEBUG("xyz"); // will be skipped at runtime
ELOG_ERROR("This is an error!"); // will be printed
...
...
ELOG_SET_LEVEL(elog::level_all); // print everything again - default
...
...
ELOG_CLEANUP();
```
A more comprehensive way of using it would be (in *main*):
``` c++
int main(int argc, char *argv[]) {
	try {
		ELOG_INIT("mylogfile.log");
		...
		...
		...
		...
		ELOG_INFO("Some ", "info ", "here! ", 4.5, -123);
		...
		...
		ELOG_DEBUG("Some ", "debug");
		...
		...
		ELOG_CLEANUP();
	} catch(const elog::exception& e) {
		std::cerr << "elog exception: " << e.what() << std::endl;
		return -1;
	} catch(const std::exception& e) {
		ELOG_ERROR("Exception: ", e.what());
		ELOG_CLEANUP();
		return -2;
	} catch(...) {
		ELOG_ERROR("Unknownw exception");
		ELOG_CLEANUP();
		return -3;
	}
}
```

## F.A.Q.s

### Is this code working only for a specific platform?
I have developed primarily on Linux (Ubuntu 16.04.3) altough I have used *std* interfaces as much as I can - this code should be used as it is on Windows/MacOS/... .

### I've has a look at the _macros_ and would like to invoke the *elog* API directly. What do the paramters for *elog::logger::init* achieve really?
Following a quick summary:
1. *fname* will specify the base filename of the log
2. *s_ordering* will have the separated writing logger I/O thread ordering the log events - adds a tiny bit more of CPU but makes the log stricly sequential
3. *e_sz* specifies how many entries are pre-allocated in the logging table - this is a control parameter that you can tune depending on how many threads and how frequently those are writing into the log
4. *roll_log_sz* specified how many bytes we can write into the logfile before it gets rolled (i.e. renamed to *log name*.0, then *log name*.1 and so on ...)

### Can you describe a bit the *inner guts* of this logging framework?
*to come*

## License
This software is licensed under the LGPLv3, so you can include the header in your source code and just say thanks - no need to release your sources (unless you modify the template that is).

### Other licensing
I'm also open to potentially let people use this under another license, better suited to commercial firms; please contact me in case you may be interested - altough at this *early* stage I don't see why you would be...

## Acknowledgements & thanks
Would like to thank very much *Carl Cook* for his inspirational speeches [2016 C++](https://www.youtube.com/watch?v=ulOLGX3HNCI) and [CppCon 2017](https://www.youtube.com/watch?v=NH1Tta7purM) - he also has a [Github](https://github.com/carlcook) account (I swear I was inspired by talks, no copy of his *variadicLogging* or *FastLogger*).

