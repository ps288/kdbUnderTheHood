# kdbUnderTheHood

A library with a mix of q and C code to inspect kdb data structures at byte level and visualise those data structures in memory.

# Requirements
* Must be used with kdb+ 4.0 or later
* Must be used with 64 bit version of kdb+ (although a 32 bit version shouldn't be difficult to implement)
* You should have a copy of the k.h header file when compiling.

# Compiling

To compile the C library into a shared object run the following:
```bash
gcc -shared -fPIC returnBytesX.c -o returnBytesX.so
```
