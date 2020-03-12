# debugging-allocator
Debugging allocator making use of base_malloc and base_free (base_malloc()and base_free are used in the same way as malloc(), calloc() and free(),
but are not language-defined, so they can offer slightly more relaxed rules) defined in basealloc.cc. Enhanced the base functions to 
support the implementation of heap usage statistics, integer overflow protection, invalid free and double-free detection, boundary write
detection, memory leak reporting and finally a heavy-hitter report. If a program makes lots of allocations, and a single line of code is
responsible for 20% or more of the total payload bytes allocated by a program, the hhtest function will report that line. Lines are checked
using sampling to maintain performance. The hhtest.cc can be used to test this functionality. Such data can be used to improve malloc functions and increase performance.


The following function, void dmalloc_get_statistics(dmalloc_statistics* stats) will call the heap usage statistics function, outputing:
alloc count: active ** total ** fail ** where ** are any numbers representing the given fields.
__________________________________________________________
Invalid free and double free detection:

Freeing a pointer not in the heap will trigger the following error message: 
MEMORY BUG: [filename]:[linenumber]: invalid free of pointer [address], not in heap
__________________________________________________________
Advanced reports and checking:
  
Upon an invalid free, the program can also tell you whether the address passed was ever allocated, and where in a current allocated block you are trying to free:

MEMORY BUG:[filename]:[linenumber]:invalid free of pointer [address], not allocated,
[filename]:[linenumber]: [address] is ** bytes inside a <size_of_allocated_block> byte region allocated here
__________________________________________________________
Leak check report:
  
The function dmalloc_print_leak_report() returns a report of every currently allocated object
in the system: 

LEAK CHECK: [filename]:[linenumber]: allocated object [address] with size ??
__________________________________________________________
Heavy-hitter report:

By use of sampling, calling 
  
dmalloc_print_heavy_hitters() will return a report of the 5 lines that were responsible for most of the memory allocated:

HEAVY HITTER: [filename]:[linenumber]: [size_allocated_by_line] bytes (~[percentage_of total size])





