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
LOG_INIT("test.log");
...
...
LOG("some ", "log ", "here ", 123, '\t', -12.45);
...
...
LOG_CLEANUP();
```

## F.A.Q.s
*Section to come*

## License
This software is licensed under the LGPLv3, so you can include the header in your source code and just say thanks - no need to release your sources (unless you modify the template that is).

### Other licensing
I'm also open to potentially let people use this under another license, better suited to commercial firms; please contact me in case you may be interested - altough at this *early* stage I don't see why you would be...
