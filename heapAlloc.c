//////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
//
///////////////////////////////////////////////////////////////////////////////
 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "heapAlloc.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           
    int size_status;
    /*
    * Size of the block is always a multiple of 8.
    * Size is stored in all block headers and free block footers.
    *
    * Status is stored only in headers using the two least significant bits.
    *   Bit0 => least significant bit, last bit
    *   Bit0 == 0 => free block
    *   Bit0 == 1 => allocated block
    *
    *   Bit1 => second last bit 
    *   Bit1 == 0 => previous block is free
    *   Bit1 == 1 => pre&heapStart;vious block is allocated
    * 
    * End Mark: 
    *  The end of the available memory is indicated using a size_status of 1.
    * 
    * Examples:
    * 
    * 1. Allocated block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 27
    *      If the previous block is free, size_status should be 25
    * 
    * 2. Free block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 26
    *      If the previous block is free, size_status should be 24
    *    Footer:
    *      size_status should be 24
    */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;

/*
 * Additional global variables may be added as needed below
 */

int emptyHeap = -1; // empty space left in heap;
blockHeader *mostRecent = NULL; // block header of the most recently alloceted block
 
/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block on success.
 * Returns NULL on failure.
 * This function should:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 and possibly adding padding as a result.
 * - Use NEXT-FIT PLACEMENT POLICY to chose a free block
 * - Use SPLITTING to divide the chosen free block into two if it is too large.
 * - Update header(s) and footer as needed.
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* allocHeap(int size) {     
    
	  	
	if(mostRecent == NULL){
	mostRecent = heapStart;
	}	
	if(emptyHeap == -1){
		emptyHeap = allocsize;
	}	
	if(size <= 0 ){
		return NULL;
	}
	
	//Add header to size
	int fullSize = size + sizeof(blockHeader);
	// Size should be a multiple of 8
	while((fullSize % 8) != 0){
		// Add padding until multiple of 8
		fullSize += 1;
	}

	// Next Fit
	blockHeader *curr = mostRecent;
	int currSize = curr -> size_status;
	
	//Loop while we are not at the end of heap
	while(currSize != 1){
	// Check if block is free
	if((currSize & 1) == 0){
		
		// Check is block is large enough
		if(fullSize > emptyHeap){
			return NULL;
		}
		else if( fullSize > currSize){
			int new = currSize;
			while(new % 8 != 0){
				new -= 1;
			}
			//get next block if curr is too small
			curr = (blockHeader*)((void*)curr + new);
			currSize = curr-> size_status;
			continue;
		}
		else{
		// Alllocate if block is large enough

		//reduce the size of emptyHeap
		emptyHeap = emptyHeap - fullSize;	
		

	   blockHeader * prev = (blockHeader *)((void*)curr - sizeof(blockHeader));
           int prevSize = prev -> size_status; // size of prev payload

	// if free block is found add 3
	// else add 1
      		 if(prevSize == 0){
			 curr -> size_status = fullSize + 3;

		 }
		else{
			curr->size_status = fullSize + 1;
		
		}	
		
		// Set the next header
		mostRecent  = (blockHeader*)((void*)curr + fullSize);
		
		//get next free block
		mostRecent -> size_status = currSize - fullSize;
		
		
		
		break;
	}
		}
	
	// If the block if full find next block
	else if((currSize & 1) != 0){
		int new = currSize;

		while(new %8 != 0){
			new -=1;
		}


		curr = (blockHeader*)((void*)curr + new);
		currSize = curr -> size_status;
		continue;
		
	}	
	}

	
    return (void*)curr + sizeof(blockHeader);
} 
 
/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - USE IMMEDIATE COALESCING if one or both of the adjacent neighbors are free.
 * - Update header(s) and footer as needed.
 */                    
int freeHeap(void *ptr) {
    int ptrAddr =(int) &ptr;	
  
    if(ptr == NULL){
	    return-1;
    }
    // multiple of 8
    if((ptrAddr % 8) != 0){
	    
	    return -1;
    }
   
    if((int)ptr < (int)heapStart || (int)ptr > ((int)heapStart + allocsize)){
	    return -1;
    }

    //gets the block of ptr 
    blockHeader *curr = (blockHeader *)((void*) ptr - sizeof(blockHeader));
    blockHeader *prev = NULL;
    blockHeader *next = NULL;

    int currSize = curr -> size_status; // size in block header
	
	
    // Check if block is alloced
    if((currSize & 1) == 1){
	currSize = currSize -1; // sub 1 for dealloc
	curr->size_status = currSize;
    }
    else{
	    // if not allocated
	    return -1;
    }

    
    	// looks for a prev free block
	prev = (blockHeader *)((void*)curr - sizeof(blockHeader)); 
	int prevSize = prev -> size_status; // size of prev payload

	if(prevSize == 0){
		prev = NULL;
	}
	else{
	//if footer is found that means that the previous is free
		prev = (blockHeader *)((void *)curr - prevSize);
		prevSize = prev -> size_status;
	}
	
	// Get size without extra bits
	while(currSize % 8 != 0){
		currSize -= 1;
	}
	//Add mem back
	emptyHeap = emptyHeap + currSize;

	next = (blockHeader *)((void*)curr + currSize);
	int nextSize = next -> size_status; // size of next block

	blockHeader *nextHolder = NULL;// used to subtract 2 from size
				       // when curr is dealloced
	
	if((nextSize & 1) == 0){
		nextHolder = next;
	}
	else{
		next = NULL;
	}
	
	// Next and prev are alloc
	if(prev == NULL && next == NULL){
	
		blockHeader *footer = (blockHeader *)((void*)curr + currSize - sizeof(blockHeader));
		if(footer -> size_status == 1){
			footer = (blockHeader *)((void*)curr + currSize - sizeof(blockHeader)-sizeof(blockHeader));
		}
		footer ->size_status = currSize;
		mostRecent = curr;
	
		// curr is free
		if(nextHolder != NULL){
		nextHolder -> size_status = (nextHolder->size_status) - 2;
		}

	return 0;
	}// if prev is alloced next is not
	else if(prev == NULL && next != NULL){

		while(nextSize %8 !=0){
			nextSize -=1;
		}

		// get size of full free block
		curr -> size_status = curr -> size_status + nextSize;
		//creates footer
		blockHeader *footer = (blockHeader*)((void*)curr + nextSize + currSize - sizeof(blockHeader));
	
		  if(footer -> size_status == 1){
                	footer = (blockHeader *)((void*)curr + nextSize + currSize - sizeof(blockHeader)-sizeof(blockHeader));
		  }


		footer -> size_status = currSize + nextSize;
		
		
		mostRecent = curr;

		return 0;
	}// next is alloced and prev is not
	else if(prev != NULL && next == NULL){

		int extraPrevBits = 0; // counts number of extra bits
		while(prevSize % 8 != 0){
			prevSize -=1;
			extraPrevBits += 1;
		}
		int newSize = prevSize + currSize;

		blockHeader *footer = (blockHeader *)((void*)curr + currSize - sizeof(blockHeader));
		
	      	if(footer -> size_status == 1){
                footer = (blockHeader *)((void*)curr + currSize - sizeof(blockHeader)-sizeof(blockHeader));
	       	}

		footer -> size_status = newSize;

		prev ->size_status = newSize + extraPrevBits;
		mostRecent = prev;

		return 0;

	}// next and prev are both free
	else if(prev != NULL && next != NULL){

		int extraPrevBit = 0;// counts number of extra bits
		
		while(prevSize % 8 != 0){
			prevSize -= 1;
			extraPrevBit += 1;
		}
		while(nextSize % 8 != 0){
			nextSize -= 1;
		}

		int newSize = currSize + prevSize + nextSize;
		
		blockHeader *footer = (blockHeader*)((void*)curr + nextSize + currSize - sizeof(blockHeader));

		 if(footer -> size_status  == 1){
                	footer = (blockHeader *)((void*)curr + nextSize + currSize - sizeof(blockHeader)-sizeof(blockHeader));
		 }
		
		footer -> size_status = newSize;
	
		prev ->size_status = newSize + extraPrevBit;
		
		mostRecent = prev;
		
		return 0;

}

    return -1;
} 
 
/*
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int initHeap(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple initHeap calls
 
    int pagesize;  // page size
    int padsize;   // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void dumpMem() {     
 
    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;

    fprintf(stdout, "************************************Block list***\
                    ********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\n");
    fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "used");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "Free");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "used");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "Free");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fprintf(stdout, "Total used size = %d\n", used_size);
    fprintf(stdout, "Total free size = %d\n", free_size);
    fprintf(stdout, "Total size = %d\n", used_size + free_size);
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fflush(stdout);

    return;  
} 
