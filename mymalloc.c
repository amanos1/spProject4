#include <stdlib.h>
#include <string.h>

#define HEAP_SIZE 1048576 //2 to the 20th power

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

// Read the size and allocated fields from addres's p
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

// Given block ptr bp computef addr'0sS •of! ti'ts headel:-. and fOoter
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// Given block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define NEXT_FREE(bp) (())

#define FIRST 0
#define  NEXT 1
#define  BEST 2

static char mode;
static char *prev_blck; //for use when the mode is set to NEXT
static char *heapTruStart; //so that we can free it
static char *heap; //Points to the first byte of heap
static char *lastBlock; //Points to last byte of heap plus 1
static char *maxAddr; // Max legal heap addr plus 1

void myinit(int allocAlg) {
	heapTruStart = malloc(HEAP_SIZE);
	heap = heapTruStart + (heapTruStart % DSIZE);
	lastBlock = heap;

	mode = allocAlg;
}

char *findFirst(size) {
}

char *findNext(size) {
}

char *findBest(size) {
}

char *realEstate(size) {
	switch(mode) {
		case FIRST:
			return findFirst(size);
		case NEXT:
			return findNext(size);
		case BEST:
			return findBest(size);
	}
}

void* mymalloc(size_t size) {
	if(size == 0) return NULL;

	size_t newSize;
	int minSize = 3 * DSIZE;

	if(size <= DSIZE)
		newSize = 4 * DSIZE;
	else
		newSize = DSIZE * ((size + ) / DSIZE);

	char *ptr;
	if ((ptr = realEstate(newSize)) == NULL) return NULL;
	place(ptr, newSize);
	return ptr;
}

/***********************************************************************/
/* THE FOLLOWING FUNCTION DETERMINES IF THE BLOCK IN QUESTION IS       */
/* ELLIGIBLE TO REMAIN A DISTICT IDENTITY OR IF IT WILL BE ASSIMILATED */
/* TO BE ONE OF US. IF IT CAN IT WILL BE MERGED WITH ITS NEIGHBOR,     */
/* PREVIOUS AFTER OR BOTH                                              */
/***********************************************************************/
void assimilate() {
}

void myfree(void* ptr) {
	if(ptr == NULL) return;

	size_t size = GET_SIZE(HDRP(ptr))

	PUT(HDRP(ptr), PACK(size, 0))
	PUT(FTRP(ptr), PACK(size, 0))
	assimilate();
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
}
