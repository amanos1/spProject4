#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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

// Given block ptr bp computef addr'0sS •of! ti'ts headel:-. and fOoter
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// Given block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define GET_NXT_PTR(p) ((p) + WSIZE)
#define GET_PRV_PTR(p) ((p) + WSIZE + WSIZE)

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
	lastBlock = heap + DSIZE;

	PUT(heap, PACK(DSIZE * 2, 1));
	prev_blck = heap;

	char *bigFreeBlk = heap + (DSIZE * 2);
	int bigFree = DSIZE * (int) ((HEAP_SIZE - (DSIZE * 2) - (heap - heapTruStart)) / DSIZE);
	PUT(bigFreeBlk, PACK(bigFree, 0));
	PUT2(GET_NXT_PTR(bigFreeBlk), (unsigned long) NULL);
	PUT2(GET_PRV_PTR(bigFreeBlk), (unsigned long) NULL);

	mode = allocAlg;
}

char *findFirst(size_t size) {
	char *currPtr = heap;
	while(currPtr != NULL) {
		if(GET_SIZE(currPtr) >= size) {
			return currPtr;
		}
		currPtr = (char *) GET_NXT_PTR(currPtr);
	}
	return NULL;
}

char *findNext(size_t size) {
	return NULL;
}

char *findBest(size_t size) {
	char *currPtr = heap;
	char *bestPtr = NULL;
	size_t lowest = HEAP_SIZE;
	while(currPtr != NULL) {
		if(GET_SIZE(currPtr) >= size) {
			if(GET_SIZE(currPtr) < lowest) {
				lowest = GET_SIZE(currPtr);
				bestPtr = currPtr;
			}
		}
		currPtr = (char *) GET_NXT_PTR(currPtr);
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
	char *prev = GET_PRV_PTR(ptr);
	char *newBlock = ptr + size;

	PUT(ptr, PACK(size, 1));
	if(diff == 0) {
		if(prev != NULL) PUT(GET_NXT_PTR(prev), (unsigned long) GET_NXT_PTR(ptr));
	} else {
		PUT(newBlock, PACK(diff, 0));
		PUT(GET_NXT_PTR(newBlock), (unsigned long) GET_NXT_PTR(ptr));
		PUT(GET_PRV_PTR(newBlock), (unsigned long) GET_PRV_PTR(ptr));
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
	return (void *)ptr;
}

/***********************************************************************/
/* THE FOLLOWING FUNCTION DETERMINES IF THE BLOCK IN QUESTION IS       */
/* ELLIGIBLE TO REMAIN A DISTICT IDENTITY OR IF IT WILL BE ASSIMILATED */
/* TO BE ONE OF US. IF IT CAN IT WILL BE MERGED WITH ITS NEIGHBOR,     */
/* PREVIOUS AFTER OR BOTH                                              */
/***********************************************************************/
void assimilate(char *addr) {
	char *prev_blk = GET_PRV_PTR(addr);
	char *nxt_blk = GET_NXT_PTR(addr);
	if(GET_ALLOC(prev_blk) == 0) {
		PUT2(prev_blk, PACK(GET_SIZE(prev_blk) + GET_SIZE(addr), 0));
		PUT2(GET_NXT_PTR(prev_blk), (unsigned long) nxt_blk);
		addr = prev_blk;
	}

	if(GET_ALLOC(nxt_blk) == 0) {
		PUT2(addr, PACK(GET_SIZE(addr) + GET_SIZE(nxt_blk), 0));
		PUT2(GET_PRV_PTR(nxt_blk), (unsigned long) GET_PRV_PTR(addr));
	}
}

void myfree(void* ptr) {
	if(ptr == NULL) return;

	size_t size = GET_SIZE(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
	assimilate(HDRP(ptr));
}

void* myrealloc(void* ptr, size_t size) {
	if(ptr >= heap || ptr >= lastBlock) return NULL;

	if(ptr == 0 && size == 0) return NULL;
	if(ptr == 0) return mymalloc(size);
	if(size == 0) {
		myfree(ptr);
		return NULL;
	}

	if(GET_ALLOC(HDRP(ptr)) != 1) return NULL;

	char *nextBlock = NEXT_BLKP(ptr);

	if(GET_ALLOC(nextBlock) == 0 
		&& size <= GET_SIZE(nextBlock) + GET_SIZE(HDRP(ptr))) {
		//take the block at nextBlock's previous pointer and make it point to nextBlocks next pointer
		//increase the size of current block
		//create another free block after our newly exteded one if we can
		return ptr;
	}

	myfree(ptr);
	return mymalloc(size);
}

void mycleanup() {
	free(heapTruStart);
	heapTruStart = NULL;
	heap = NULL;
}

void printBlocks() {
	void *pointer = heapTruStart;

	while(pointer != NULL) {
		printf("%lu %i %i\n", (unsigned long) pointer, GET_SIZE(pointer), GET_ALLOC(pointer));
		pointer = GET_NXT_PTR(pointer);
	}
}
