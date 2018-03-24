Dynamic Memory Allocator

Description
	- This project is an implementation of a best-fit explicit free list dynamic memory 
	  allocator which handles padding and splinters. The program dives into the heap to
	  handle memory requests from the user and the block of memory which best fits the
	  request of the user is chosen to contain the data. The program also performs coalescing
	  to merge blocks of free data.

Functions
	sf_malloc
		- functions exactly like malloc
	sf_realloc
		- functions exactly like realloc
	sf_free
		- functions exactly like free
	sf_info
		- shows the following information:
			-allocatedBlocks	- total number of allocated memory blocks
			-splinterBlocks		- total number of allocated memory blocks containing splinters
			-padding		- total memory consumed for padding
			-splintering		- total memory consumed by splinters
			-coalesces		- total number of coalescing made
			-peakMemoryUtilization  - when memory is most used efficiently