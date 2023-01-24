This folder contains the implementations of general purpose data structures and definitions that are useful through all of the codebase. Some of the more used data structures implemented in it are:
	- BPAlloc  Bump-pointer allocator. It's useful during the compilation 
	           phase because lots of small objects need to be allocated and 
	           then freed at the same time when the final executable is produced.
	
	- Error    Structure useful to report errors to function callers.
	
	- Source   String object that is used in place of raw strings to move 
	           source code around.