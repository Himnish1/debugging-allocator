# debugging-allocator
Debugging allocator making use of base_malloc and base_free (base_malloc()and base_free (are used in the same way as malloc()and free(),
they are not language-defined, so they can offer slightly more relaxed rules) defined in basealloc.cc. Enhanced the base functions to 
support the implementation of heap usage statistics, integer overflow protection, invalid free and double-free detection, boundary write
detection, memory leak reporting and finally a heavy-hitter report. If a program makes lots of allocations, and a single line of code is
responsible for 20% or more of the total payload bytes allocated by a program, the hhtest function will report that line. Lines are checked
using sampling to maintain performance. The hhtest.cc can be used to test this functionality. Such data can be used to improve malloc functions and increase performance.
