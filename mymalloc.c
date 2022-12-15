#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "mymalloc.h"

#define HEAP_SIZE 1048575 //2 to the 20th power

// Basic constants
#define WSIZE 4 //Word and header/footer size (bytes)
#define DSIZE 8 //Double word size (bytes)
#define CHUNKSIZE (1<<12) // Extend h~ap"by this amount (bytes)

#define MAX(x, y) ((x) > (y)? (x) : (y))

// Pack a size and allocated bit into q word
#define PACK(size, alloc) ((size) | (alloc))

// Read and write a word at address p
#define GET(p)	(*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))
#define PUT2(p, val) (*(unsigned long *)(p) = (val))

// Read the size and allocated fields from addres's p
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// Given block ptr bp computef address of its header and footer
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// Given block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define NXT_PTR(p) ((p) + WSIZE)
#define PRV_PTR(p) ((p) + WSIZE + DSIZE)

#define GET_NXT_PTR(p) (*(unsigned long *)(NXT_PTR(p)))
#define GET_PRV_PTR(p) (*(unsigned long *)(PRV_PTR(p)))

#define FIRST 0
#define  NEXT 1
#define  BEST 2

static char mode;
static void *prev_blck; //for use when the mode is set to NEXT
static void *heapTruStart; //so that we can free it
static void *heap; //Points to the first byte of heap
static void *lastBlock; //Points to last byte of heap plus 1

/***********************************************************************/
/*  */
/***********************************************************************/
void myinit(int allocAlg) {
	heapTruStart = malloc(HEAP_SIZE);
	heap = heapTruStart + ((unsigned long) heapTruStart % DSIZE);

	PUT(heap, PACK(DSIZE * 3, 1));
	PUT(FTRP(heap + WSIZE), PACK(DSIZE * 3, 1));
	prev_blck = heap;
	PUT2(PRV_PTR(heap), (unsigned long) NULL);

	char *bigFreeBlk = heap + (DSIZE * 3);
	int bigFree = DSIZE * (int) ((HEAP_SIZE - (DSIZE * 3) - (heap - heapTruStart)) / DSIZE);
	PUT(bigFreeBlk, PACK(bigFree, 0));
	PUT(FTRP(bigFreeBlk + WSIZE), PACK(bigFree, 0));
	PUT2(NXT_PTR(bigFreeBlk), (unsigned long) NULL);
	PUT2(PRV_PTR(bigFreeBlk), (unsigned long) heap);
	PUT2(NXT_PTR(heap), (unsigned long) bigFreeBlk);
	
	lastBlock = bigFreeBlk;
	mode = allocAlg;
}

	


/***********************************************************************/
/* THE FOLLOWING FUNCTION DETERMINES IF THE BLOCK IN QUESTION IS       */
/* ELLIGIBLE TO REMAIN A DISTICT IDENTITY OR IF IT WILL BE ASSIMILATED */
/* TO BE ONE OF US. IF IT CAN IT WILL BE MERGED WITH ITS NEIGHBOR,     */
/* PREVIOUS AFTER OR BOTH                                              */
/***********************************************************************/
int assimilate(char *addr) {
	void *prev_blk = HDRP(PREV_BLKP(addr + WSIZE));
	void *nxt_blk =  HDRP(NEXT_BLKP(addr + WSIZE));
	int unique = 1;
	

	if(GET_PRV_PTR(addr) == (unsigned long) prev_blk && GET_ALLOC(prev_blk) == 0) {
		int newSize = GET_SIZE(prev_blk) + GET_SIZE(addr);
		PUT(prev_blk, PACK(newSize, 0));
		PUT(FTRP(prev_blk+WSIZE), PACK(newSize, 0));

		PUT2(NXT_PTR(prev_blk), (unsigned long) nxt_blk);
		addr = prev_blk;
		if(lastBlock == addr) lastBlock = prev_blk;
		unique = 0;
	}

	if(GET_NXT_PTR(addr) ==  (unsigned long) nxt_blk && GET_ALLOC(nxt_blk) == 0) {
		if(nxt_blk == prev_blck){
			prev_blck = addr;
		}
		int newSize = GET_SIZE(nxt_blk) + GET_SIZE(addr);
		PUT(addr, PACK(newSize, 0));
		PUT(FTRP(addr+WSIZE), PACK(newSize, 0));
		PUT2(NXT_PTR(addr), (unsigned long) GET_NXT_PTR(nxt_blk));
		if(GET_NXT_PTR(addr) != 0) PUT2(PRV_PTR(GET_NXT_PTR(addr)), (unsigned long) addr);
		if(lastBlock == nxt_blk) lastBlock = addr;
		unique = 0;
	}

	return unique;
}


char *findFirst(size_t size) {
	char *currPtr = (char *) GET_NXT_PTR(heap);


	while(currPtr != NULL && currPtr <= (char *) lastBlock) {

		if(GET_SIZE(currPtr) >= size) {
			return currPtr;
		}
		currPtr = (char *) GET_NXT_PTR(currPtr);
	}
	return NULL;
}

char *findNext(size_t size) {
	char *iterator = prev_blck;

	while(iterator != NULL){
		if(GET_SIZE(iterator) >= size){
			prev_blck = iterator + size;
			return iterator;
		}
		iterator = (char *) GET_NXT_PTR(iterator);
		if(iterator == NULL || (void *) iterator > lastBlock){
			iterator = heap;
			prev_blck = iterator;
		}
		/*
		if(iterator == prev_blck){
			//You have made a loop and not found a suitable location
			prev_blck = iterator;
			return NULL;
		}
		*/
	}
	return NULL;
}

char *findBest(size_t size) {
	char *currPtr = (char *) GET_NXT_PTR(heap);
	char *bestPtr = NULL;
	size_t lowest = HEAP_SIZE;
	while(currPtr != NULL) {
		if(GET_SIZE(currPtr) >= size) {
			if(GET_SIZE(currPtr) < lowest) {
				lowest = GET_SIZE(currPtr);
				bestPtr = currPtr;
			}
		}
		currPtr = (void *) GET_NXT_PTR(currPtr);
	}
	return bestPtr;
}

char *realEstate(size_t size) {
	switch(mode) {
		case FIRST:
			return findFirst(size);
		case NEXT:
			return findNext(size);
		default:
			return findBest(size);
	}
}

/************************************************/
/* Creates a new block at the specified address */
/* of the specified size                        */
/************************************************/

void place(void *ptr, size_t size) {
	int diff = GET_SIZE(ptr) - size;
	void *prev = (void *) GET_PRV_PTR(ptr);
	void *next = (void *) GET_NXT_PTR(ptr);
	void *newBlock = ptr + size;
	//printBlocks();
	//printf("%lu %lu\n", prev, next);
	
	PUT(ptr, PACK(size, 1));
	PUT(FTRP(ptr + WSIZE), PACK(size, 1));
	//printBlocks();
	if(diff == 0) {
		if(prev != NULL) PUT2(NXT_PTR(prev), (unsigned long) GET_NXT_PTR(ptr));
		if(next != NULL) PUT2(PRV_PTR(next), (unsigned long) GET_PRV_PTR(ptr));
	} else {

		PUT(newBlock, PACK(diff, 0));
		PUT(FTRP(newBlock + WSIZE), PACK(diff, 0));
		PUT2(NXT_PTR(newBlock), (unsigned long) next);
						
		PUT2(PRV_PTR(newBlock), (unsigned long) prev);

		PUT2(NXT_PTR(GET_PRV_PTR(ptr)), (unsigned long) newBlock);
		if(GET_NXT_PTR(ptr) != 0) PUT2(PRV_PTR(GET_NXT_PTR(ptr)), (unsigned long) newBlock);
		assimilate(newBlock);


	}

	if(lastBlock <= newBlock){
		lastBlock = newBlock;
	}
}


void* mymalloc(size_t size) {
	if(size == 0) return NULL;

	size_t newSize;
	int minSize = 3 * DSIZE;

	if(size <= minSize)
		newSize = 4 * DSIZE;
	else
		newSize = DSIZE * ((size + 3*DSIZE + (DSIZE-1)) / DSIZE);

	char *ptr;
	if ((ptr = realEstate(newSize)) == NULL) return NULL;
	
	place(ptr, newSize);



	return (void *)(ptr + WSIZE);
}



void myfree(void* ptr) {
	if(ptr <= heap || ptr >= lastBlock) return;

	size_t size = GET_SIZE(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));

	void *p = heap;
	while(GET_NXT_PTR(p) < (unsigned long) ptr) {
		p = (void *) GET_NXT_PTR(p);
	}

	char *nextP = (char *) GET_NXT_PTR(p);	

	PUT2(PRV_PTR(HDRP(ptr)), (unsigned long) p);
	PUT2(NXT_PTR(HDRP(ptr)), (unsigned long) nextP);
	PUT2(PRV_PTR(nextP), (unsigned long) HDRP(ptr));
	PUT2(NXT_PTR(p),     (unsigned long) HDRP(ptr));

	assimilate(HDRP(ptr));

} 



void* myrealloc(void* ptr, size_t size) {
	if(ptr == NULL && size <= 0) return NULL;
	if(ptr == NULL) return mymalloc(size);
	if(size <= 0) {
		myfree(ptr);
		return NULL;
	}

	if(ptr <= heap || ptr >= lastBlock) return NULL;

	if(GET_ALLOC(HDRP(ptr)) != 1) return NULL;


	size_t newSize;
	int minSize = 3 * DSIZE;

	if(size <= minSize)
		newSize = 4 * DSIZE;
	else
		newSize = DSIZE * ((size + 3*DSIZE + (DSIZE-1)) / DSIZE);

	char *nextBlock = HDRP(NEXT_BLKP(ptr));

	if(GET_ALLOC(nextBlock) == 0 
		&& newSize <= GET_SIZE(nextBlock) + GET_SIZE(HDRP(ptr))) {

		PUT(HDRP(ptr), PACK(GET_SIZE(nextBlock) + GET_SIZE(HDRP(ptr)), 0));
		PUT2(NXT_PTR(HDRP(ptr)), GET_NXT_PTR(nextBlock));

		PUT2(PRV_PTR(HDRP(ptr)), GET_PRV_PTR(nextBlock));
		PUT2(NXT_PTR(GET_PRV_PTR(nextBlock)), (unsigned long) HDRP(ptr));
		if(GET_NXT_PTR(nextBlock) != 0) PUT2(PRV_PTR(GET_NXT_PTR(nextBlock)), (unsigned long) HDRP(ptr));
		//printf("%lu %lu %lu\n", nextBlock, GET_NXT_PTR(nextBlock), GET_PRV_PTR(nextBlock));
		place(HDRP(ptr), newSize);
	
		return ptr;
	}

	myfree(ptr);
	//return mymalloc(size); 

	char *ptr2 = realEstate(newSize);

	if(ptr2 != NULL) place(ptr2, newSize);
	
	return (void *) (ptr + WSIZE);
}

void mycleanup() {
	free(heapTruStart);
	heapTruStart = NULL;
	heap = NULL;
}

void printBlocks() {
	void *pointer = heap;

	while(pointer <= lastBlock) {
		if(pointer != heap && GET_ALLOC(pointer) == 1)
			printf("%lu %i %i\n", (unsigned long) pointer, GET_SIZE(pointer), GET_ALLOC(pointer));
		else
			printf("%lu %i %i\n", (unsigned long) pointer, GET_SIZE(pointer), GET_ALLOC(pointer));
		pointer = (void *) HDRP(NEXT_BLKP(pointer + WSIZE));
		if(pointer == NULL) break;
	}

	printf("\n");
}
