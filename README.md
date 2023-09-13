# Memory_Management_Library
A highly scalable memory management library similar to the memory management implemented in stdlib in C (and new respectively in C++) implementing the respective calls of malloc, realloc, calloc, free and new/delete. The memory management library files are included in the lib directory.

The implementation is based on the paper: [Scalable locality-conscious multithreaded memory allocation](https://dl.acm.org/doi/10.1145/1133956.1133968).

Made for the Advanced Topics in Systems Software course of University of Thessaly.


## Instructions:

To compile shared libraries and create test executables use:
```
make
```

### memory_manager.so : 
> Intended to be used with LD_PRELOAD to override malloc, realloc, calloc, free from files that use these typical functions (generally can be dangerous if any other function except the ones defined is used in the code to allocate memory and then free is used)
### memory_manager_u.so : 
> Intended to be used with LD_LIBRARY_PATH for files that include the header file memory_manager.h to use the defined functions umalloc, urealloc, ucalloc, ufree. Can also be used with C++ code to overload new,delete (file must have been compiled with -L./lib/ -l:memory_manager_u.so).

To run test (which is the alloc_test bench):
```
 LD_LIBRARY_PATH=./lib/ ./test
```

To run the stress-ng memory stress benchmark inside the stress-ng directory:

```
 cd stress-ng/
 make
 LD_PRELOAD=../lib/memory_manager.so ./stress-ng --malloc 4 -t 5s
```

To run remote_free_test:
```
 LD_LIBRARY_PATH=./lib/ ./remote_free_test
```

To run partial_test:
```
 LD_LIBRARY_PATH=./lib/ ./partial_test
```

### Contributors:

Ippokratis Koukoulis

Christos Nanis