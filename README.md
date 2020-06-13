# Thread Safe Memory Manager

A memory manager, or simply, my implementation of "alloc()" and "free()".


Memory is managed using a linked list, a list of both free and used memory chunks are contained within. If two adjencet blocks are free, they will be merged.


I used the pthreads library and utilized mutex locks to made the implementation thread safe.


You may also specify the allocation algorithm used, i.e. First-Fit, Best-Fit, Next-Fit.
