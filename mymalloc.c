#include <stdlib.h>
#include <string.h>

void *heap;

void myinit(int allocAlg) {
}

void* mymalloc(size_t size) {
	if(size == 0) return NULL;
}

void myfree(void* ptr) {
	if(ptr == NULL) return;
}

void* myrealloc(void* ptr, size_t size) {
	//if ptr is null, call free; if size is 0, call malloc
	//fist we determine if the ptr points to memory in our heap
	//then we check if mem can be reallocated in place of if we will have to migrate based on size
	//then we call free on old address and malloc on new address
}

void mycleanup() {
}
