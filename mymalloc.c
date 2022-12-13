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

#define NXT_PTR(p) ((p) + WSIZE)
#define PRV_PTR(p) ((p) + WSIZE + WSIZE)

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

char *findFirst(size_t size) {
	char *currPtr = heap;
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
			prev_blck = iterator;
			return iterator;
		}
		iterator = (char *) GET_NXT_PTR(iterator);
		if(iterator == NULL || (void *) iterator > lastBlock){
			iterator = heap;
		}
		if(iterator == prev_blck){
			//You have made a loop and not found a suitable location
			prev_blck = iterator;
			return NULL;
		}
	}
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

	PUT(ptr, PACK(size, 1));
	PUT(FTRP(ptr + WSIZE), PACK(size, 1));
	if(diff == 0) {
		printf("%lu %lu %lu\n\n", heap, lastBlock, ptr);
		if(prev != NULL) PUT2(NXT_PTR(prev), (unsigned long) GET_NXT_PTR(ptr));
		if(next != NULL) PUT2(PRV_PTR(next), (unsigned long) GET_PRV_PTR(ptr));
	} else {
		PUT(newBlock, PACK(diff, 0));
		PUT(FTRP(newBlock + WSIZE), PACK(diff, 0));
		PUT2(NXT_PTR(newBlock), (unsigned long) GET_NXT_PTR(ptr));
		PUT2(PRV_PTR(newBlock), (unsigned long) GET_PRV_PTR(ptr));
		PUT2(NXT_PTR(GET_PRV_PTR(ptr)), newBlock);
	}

	if(lastBlock <= ptr) {
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
	printf("%lu hhh\n", (unsigned long) ptr);
	return (void *)(ptr + WSIZE);
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

	if(GET_PRV_PTR(addr) == (unsigned long) prev_blk) {
		PUT(prev_blk, PACK(GET_SIZE(prev_blk) + GET_SIZE(addr), 0));
		PUT2(NXT_PTR(prev_blk), (unsigned long) nxt_blk);
		addr = prev_blk;
		if(lastBlock == addr) lastBlock = prev_blk;
		unique = 0;
	}

	if(GET_NXT_PTR(addr) ==  (unsigned long) nxt_blk) {
		PUT(addr, PACK(GET_SIZE(addr) + GET_SIZE(nxt_blk), 0));
		PUT2(NXT_PTR(addr), (unsigned long) GET_NXT_PTR(nxt_blk));
		if(lastBlock == nxt_blk) lastBlock = addr;
		unique = 0;
	}

	return unique;
}

void myfree(void* ptr) {
	if(ptr <= heap || ptr >= lastBlock) return;

	size_t size = GET_SIZE(HDRP(ptr));

	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));

	if(assimilate(HDRP(ptr)) == 1) {
		void *p = heap;
		while(p < ptr) {
			if(GET_NXT_PTR(p) > (unsigned long) ptr) {
				PUT2(NXT_PTR(ptr), (unsigned long) GET_NXT_PTR(p));
				PUT2(PRV_PTR(ptr), (unsigned long) p);
				PUT2(NXT_PTR(p),   (unsigned long) ptr);
			}
			p = (void *) GET_NXT_PTR(p);
		}
	}
}

void* myrealloc(void* ptr, size_t size) {
	if(ptr <= heap || ptr >= lastBlock) return NULL;

	if(ptr == NULL && size <= 0) return NULL;
	if(ptr == NULL) return mymalloc(size);
	if(size <= 0) {
		myfree(ptr);
		return NULL;
	}

	if(GET_ALLOC(HDRP(ptr)) != 1) return NULL;

	size_t newSize;
	int minSize = 3 * DSIZE;

	if(size <= minSize)
		newSize = 4 * DSIZE;
	else
		newSize = DSIZE * ((size + 3*DSIZE + (DSIZE-1)) / DSIZE);

	char *nextBlock = NEXT_BLKP(HDRP(ptr));

	if(GET_ALLOC(nextBlock) == 0 
		&& newSize <= GET_SIZE(nextBlock) + GET_SIZE(HDRP(ptr))) {
		place(HDRP(ptr), newSize);
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
	void *pointer = heap;

	while(pointer <= lastBlock) {
		printf("%lu %i %i\n", (unsigned long) pointer, GET_SIZE(pointer), GET_ALLOC(pointer));
		pointer = (void *) HDRP(NEXT_BLKP(pointer + WSIZE));
		if(pointer == NULL) break;
	}
}
