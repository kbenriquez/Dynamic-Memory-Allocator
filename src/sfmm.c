/**
Author: Kyle Marcus Enriquez
Dynamic Memory Allocator - Uses an explicit free list with best-fit searching for free memory blocks.
						   Handles overhead memory for headers and footers and also for padding/alignment
						   and splinters.
 */
#include "sfmm.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

sf_free_header* freelist_head = NULL;
int max = 4;
static int i = 0;

sf_header *head = NULL;
sf_footer *foot = NULL;
sf_free_header *headPtr = NULL;
int set = 0;

void *startOfHeap;
void *endOfHeap;

//info *stats;
static int info_alloc = 0;
static int info_splinters = 0;
static int info_padding = 0;
static int info_splintering = 0;
static int info_coalesces = 0;

static double totalPayload = 0;
static double aggPayload = 0;
//static double info_peakMem = 0;


//ignore: PAGE TAB 111
/*Function to allocate memory*/
void *sf_malloc(size_t size) {
	void *ptr = NULL;
	void *ptr2 = NULL;

	if(size == 0){
		errno = EINVAL;
		return NULL;
	}

	int foundBlock = 0;				//If a block that can hold the allocation is found, set to 1
	int least = 99999;				//For comparing block sizes for finding the best (smallest) block size that fits
	void *best = NULL;				//If no exact size found, 'best' will be the next closest size
	int canSplit = 0;				//If block can be split after allocating
	int isHead = 0;					//If block to be allocated is the head of the list
	int pad = 16 - (size%16);		//For calculating the padding size
	if(size%16 == 0)
		pad = 0;

	/*Initialize the free list, called only ONCE*/
	if(freelist_head == NULL){
		if(max > 0){
			freelist_head = sf_sbrk(4096);
			if(set == 0){
				startOfHeap = freelist_head;
				set++;
			}
			max--;
			++i;
			freelist_head->header.alloc = 0;
			freelist_head->header.splinter = 0;
			freelist_head->header.block_size = 4096>>4;
			freelist_head->next = NULL;
			freelist_head->prev = NULL;
			ptr = (char*)freelist_head;
			headPtr = freelist_head;

			/*set ptr2 to point to the footer of the current block*/
			ptr2 = (char*)ptr+(headPtr->header.block_size<<4)-8;
			endOfHeap = ptr2;
			foot = ptr2;
			foot->block_size = freelist_head->header.block_size;
			foot->alloc = 0;
		}
	}

	while(1){


	/*Set headPtr to head of freelist_head*/
	headPtr = freelist_head;

	/*Initialize comparisons*/
	best = headPtr;


		/*Iterate through the free list*/
		while(headPtr != NULL){

			/*If EXACT match found*/
			if(size+16+pad == headPtr->header.block_size<<4){

				/*If match found, allcoate block*/
				if(headPtr->header.alloc == 0){

					/*set ptr to point to the payload*/
					ptr = headPtr;
					ptr = ptr + 8;

					/*set ptr2 to point to the footer*/
					ptr2 = headPtr;
					ptr2 = (char*)ptr2+(headPtr->header.block_size<<4)-8;


					if(headPtr == freelist_head){
						freelist_head->prev = NULL;

						freelist_head = headPtr->next;
					}
					else if(ptr2 == endOfHeap){
						sf_free_header *thePrev = headPtr->prev;
						thePrev->next = headPtr->next;
					}
					else{
						sf_free_header *theNext = headPtr->next;
						sf_free_header *thePrev = headPtr->prev;
						theNext->prev = thePrev;
						thePrev->next = theNext;
					}

					/*Set header bits*/
					headPtr->header.block_size = (size+16+pad)>>4;
					headPtr->header.requested_size = size;
					headPtr->header.padding_size = pad;
					headPtr->header.alloc = 1;
					headPtr->header.splinter = 0;
					headPtr->header.splinter_size = 0;


					/*Set footer bits*/
					foot = ptr2;
					foot->alloc = 1;
					foot->splinter = 0;
					foot->block_size = headPtr->header.block_size;

					totalPayload = totalPayload + size;
					if(totalPayload > aggPayload)
						aggPayload = totalPayload;
					return ptr;
				}
			}


			/*If exact match not found, check if block can at least fit allocation*/
			else{

				/*If block can fit allocation*/
				if(headPtr->header.block_size<<4 > size+pad+16){
					foundBlock = 1;
					if(least > headPtr->header.block_size<<4){
						least = headPtr->header.block_size<<4;
						best = headPtr;
					}
				}
			}
			headPtr = headPtr->next;
		}


		/*If we have reached the end of the free list and there is a block that can hold the allocation*/
		if(foundBlock == 1){
			info_alloc++; //allocalloc

			/*set headPtr to point to the best block*/
			headPtr = best;

			/*Initialize original_size as the size of the free block before allocation*/
			int original_size = headPtr->header.block_size<<4;
			int newListSize = original_size - (size+pad+16);

			/*set ptrToNext to point to headPtr->next*/
			void *ptrToNext = headPtr->next;

			/*set ptrToPrev to point to headPtr->prev*/
			void *ptrToPrev = headPtr->prev;


			/*Is the current block also the head of the free list?*/
			if(headPtr->prev == NULL){
				isHead = 1;
			}


			/*Splinter handling*/
			if(original_size-(size+pad+16) < 32){
				canSplit = 0;
			}
			if(original_size-(size+pad+16) >= 32){
				headPtr->header.splinter = 0;
				headPtr->header.splinter_size = 0;
				canSplit = 1;
			}

			/*Set header bits for allocated block*/
			headPtr->header.block_size = (size+16+pad)>>4;
			headPtr->header.requested_size = size;
			headPtr->header.padding_size = pad;
			headPtr->header.alloc = 1;

			info_padding = info_padding + pad;

			/*Set ptr to point to payload*/

			ptr = headPtr;
			void *ptrSplint = headPtr;
			ptr = ptr + 8;

			/*Set ptr2 to point to footer*/
			ptr2 = headPtr;
			ptr2 = (char*)ptr2+(headPtr->header.block_size<<4)-8;
			foot = ptr2;

			/*set footer bits for allocated block*/
			foot->alloc = 1;
			foot->block_size = headPtr->header.block_size;

			/*If the block is big enough to split without having splinters*/
			if(canSplit == 1){
				headPtr->header.splinter_size = 0;
				headPtr->header.splinter = 0;

				/*Set ptr3 to point to the head of the head of the block right next to the allocated block*/
				void *ptr3 = ptr2 + 8;

				/*headPtr2 points to the block right next to the allocated block*/
				sf_free_header *headPtr2 = ptr3;

				/*Set alloc bits of the header of the block next to allocated block*/
				headPtr2->header.alloc = 0;
				headPtr2->header.block_size = newListSize>>4;

				headPtr2->next = ptrToNext;
				headPtr2->prev = ptrToPrev;


				/*Set ptr4 to point to the foot of the block right next to the allocated block*/
				void* ptr4 = NULL;
				ptr4 = (char*)ptr3+(headPtr2->header.block_size<<4)-8;
				foot = ptr4;
				foot->alloc = 0;
				foot->splinter = 0;
				foot->block_size = headPtr2->header.block_size;

				/*Get next and prev of allocated block*/


				/*headPtr now points to the head of the block after the allocated block*/
				headPtr = ptr3;

				/*
					If headPtr had NULL for its prev, then that means that headPtr was the freelist_head. Therefore, if we are
					splitting the block of the freelist_head, the block right next to it becomes the new freelist_head.
				*/

				/*If our candidate is the head of the free list*/
				if(isHead == 1){
					freelist_head = headPtr2;
					if(ptrToNext != NULL){
						sf_free_header *headOfNext = ptrToNext;
						headOfNext->prev = headPtr2;
					}
					else{
						freelist_head->next = NULL;
						freelist_head->prev = NULL;
					}
				}

				/*If our candidate is NOT the head of the free list*/
				else{
					headPtr2->next = ptrToNext;
					headPtr2->prev = ptrToPrev;

					sf_free_header *headOfPrev = ptrToPrev;
					headOfPrev->next = headPtr2;
					if(ptrToNext != NULL){
						sf_free_header *headOfNext = ptrToNext;
						headOfNext->prev = headPtr2;
					}
				}
			}

			/*If allocating the block will result in a splinter*/
			else{

				info_splinters++;

				/*ptrSplint points to the header*/
				ptr = ptrSplint;
				headPtr = ptrSplint;
				ptr = ptr + 8;

				/*set ptr2 to point to the footer*/
				ptr2 = ptrSplint;
				ptr2 = (char*)ptr2+original_size-8;



				if(headPtr == freelist_head){
					freelist_head->prev = NULL;

					freelist_head = headPtr->next;
					headPtr->next = NULL;
					freelist_head->prev = NULL;
				}
				else if(ptr2 == endOfHeap){
					sf_free_header *thePrev = headPtr->prev;
					thePrev->next = headPtr->next;
				}
				else{
					sf_free_header *theNext = headPtr->next;
					sf_free_header *thePrev = headPtr->prev;
					theNext->prev = thePrev;
					thePrev->next = theNext;
				}

				/*Set header bits*/
				headPtr->header.block_size = original_size>>4;
				headPtr->header.requested_size = size;
				headPtr->header.padding_size = pad;
				headPtr->header.alloc = 1;
				headPtr->header.splinter = 1;
				headPtr->header.splinter_size = original_size-(size+pad+16);

				info_splintering = info_splintering + original_size-(size+pad+16);
				info_padding = info_padding + pad;

				/*Set footer bits*/
				foot = ptr2;
				foot->alloc = 1;
				foot->splinter = 1;
				foot->block_size = headPtr->header.block_size;
			}

			/*return the address of the payload of the allocated block*/
			totalPayload = totalPayload + size;
			if(totalPayload > aggPayload)
				aggPayload = totalPayload;
			return ptr;
		}

		/*If the freelist has no block that can fit the allocation*/
		else{

			/*set ptr5 to point to the start of the new page*/
			void* ptr5 = sf_sbrk(4096);
			sf_free_header *pageHead = ptr5;
			++i;
			/*check if sf_sbrk has already been called 4 times*/
			max--;
			if(max < 0){
				printf("ERROR: Memory overflow: sf_sbrk has already been called 4 times already!\n");
				errno = ENOMEM;
				return (void*) -1;
			}

			/*set foot to point to the address right before the page head*/
			foot = ptr5-8;

			/*If the block before the new page is free, coalesce the two blocks together*/
			if(foot->alloc == 0){

				/*COALESCING*/

				/*set ptr5 to point to the head of the new page*/
				pageHead->header.block_size = 4096>>4;
				pageHead->header.alloc = 0;

				/*set ptr7 to point to the foot of the pageHead*/
				void *ptr7 = (char*)pageHead + (pageHead->header.block_size<<4)-8;
				endOfHeap = ptr7;
				sf_footer *pageFoot= ptr7;
				pageFoot->block_size = pageHead->header.block_size;
				pageFoot->alloc = 0;

				/*set ptr6 to point to the head of the previous free block*/
				void *ptr6 = (char*)foot-(foot->block_size<<4)+8;
				sf_free_header *prevHead = ptr6;



				int combinedSize = (prevHead->header.block_size<<4) + (pageHead->header.block_size<<4);

				/*If coalescing to existing free block, then the next and prev must already be set*/
				prevHead->next = NULL;
				pageHead->prev = NULL;
				prevHead->header.block_size = combinedSize>>4;
				pageFoot->block_size = combinedSize>>4;

				info_coalesces++;

			}

			/*If the block before the newPage is allocated*/
			else{
				sf_free_header *seeker = freelist_head;
				while(seeker->next != NULL){
					seeker = seeker->next;
				}
				seeker->next = ptr5;
				pageHead->prev = seeker;
			}
		}
	}
		errno = EINVAL;
		return NULL;
}

//ignore: PAGE TAB 222
void *sf_realloc(void *ptr, size_t size) {


	if(ptr < startOfHeap || ptr > endOfHeap){
		printf("ERROR: ptr is out of range!\n");
		errno = EINVAL;
		return NULL;
	}
	if(size == 0){
		printf("ERROR: Size is zero!\n");
		sf_free(ptr);
		return NULL;
	}
	if(size > (4096*4)){
		printf("ERROR: Size is too big!\n");
		errno = EINVAL;
		return NULL;
	}


	sf_free_header *currentHead = ptr-8;
	void *ptr2 = (char*)currentHead + (currentHead->header.block_size<<4)-8;
	sf_footer *currentFoot = ptr2;
	sf_free_header *nextHead = ptr2 + 8;

	int actualPayload = (currentHead->header.block_size<<4) - currentHead->header.padding_size - currentHead->header.splinter_size - 16;

	if(currentHead->header.alloc == 0 || currentFoot->alloc == 0){
		printf("ERROR: Blocks are free!\n");
		errno = EINVAL;
		return NULL;
	}
	if(currentHead->header.splinter != currentFoot->splinter){
		printf("ERROR: Splinter bits are not the same!\n");
		errno = EINVAL;
		return NULL;
	}
	if(currentHead->header.block_size != currentFoot->block_size){
		printf("ERROR: Block sizes are not the same!\n");
		errno = EINVAL;
		return NULL;
	}


	/*Get the destination of the new memory block first*/


	/*Get the payload of the currentHead*/
	int currentHeadPL = (currentHead->header.requested_size)+(currentHead->header.padding_size);

	/*Get the padding for the new size*/
	int newPad = 16 - (size%16);
	if(size%16 == 0)
		newPad = 0;
	int newBlockSize = size + newPad + 16;


	/*Are we reallocing the same size?*/
	if(size == currentHeadPL){
		return ptr;
	}



	if(newBlockSize == currentHead->header.block_size<<4){
		void *returnSame = (char*)currentHead + 8;

		/*Just update header/footer contents and then return*/
		currentHead->header.requested_size = size;
		currentHead->header.padding_size = newPad;
		info_padding = info_padding + newPad;

		if(currentHead->header.splinter == 1){
			int temp = currentHead->header.splinter_size;
			currentHead->header.splinter_size = ((currentHead->header.block_size<<4) - (currentHead->header.requested_size)-(currentHead->header.padding_size) - 16);
			if(currentHead->header.splinter_size == 0){
				currentFoot->splinter = 0;
				currentHead->header.splinter = 0;

				info_splintering = info_splintering - temp;
				info_splinters--;
			}
			else{
				info_splintering = info_splintering - temp + currentHead->header.splinter_size;
			}
		}
		if(size >= actualPayload)
			totalPayload = totalPayload + size - actualPayload;
		else
			totalPayload = totalPayload + actualPayload - size;
		if(totalPayload > aggPayload)
			aggPayload = totalPayload;
		return returnSame;
	}


	/*If we are reallocing to a smaller size*/
	if(size < currentHeadPL){

		/*get the original block size*/
		int original_size = currentHead->header.block_size<<4;

		/*get the leftover from subtracting the original block size to the new block size*/
		int leftOver = original_size - (size+newPad+16);

		/*If the leftover is big enough to be a valid free block, split the blocks*/
		if(original_size - (size + newPad + 16) >= 32){


			void *ptrToNewFoot = (char*)currentHead + newBlockSize - 8;
			sf_footer *newCurrFoot = ptrToNewFoot;

			int temp = currentHead->header.padding_size;

			int oldSplinterSize = currentHead->header.splinter_size;

			/*Set header contents for the allocated block*/
			currentHead->header.block_size = newBlockSize>>4;
			currentHead->header.padding_size = newPad;
			currentHead->header.requested_size = size;
			currentHead->header.splinter_size = 0;
			currentHead->header.splinter = 0;

			info_padding = info_padding - temp + currentHead->header.padding_size;
			if(oldSplinterSize != 0){
				info_splinters--;
				info_splintering = info_splintering - oldSplinterSize;
			}

			/*Set footer contents for the allocated block*/
			newCurrFoot->block_size = newBlockSize>>4;
			newCurrFoot->alloc = 1;
			newCurrFoot->splinter = 0;

			/*set newNextHead to point to the new head of the free block*/
			void *ptrToNewNextHead = ptrToNewFoot + 8;
			sf_free_header *newNextHead = ptrToNewNextHead;

			/*Set contents for newNextHead*/
			newNextHead->header.block_size = leftOver>>4;
			newNextHead->header.alloc = 0;


			/*Now to take care of coalescing/splitting the block*/

			/*If the nextHead is an invalid block/allocated block and we cannot coalesce*/
			if(nextHead->header.alloc == 1 || currentFoot == endOfHeap){


				/*Set contents for nextFoot*/
				void *ptrToNewNextFoot = newNextHead;
				ptrToNewNextFoot = (char*)ptrToNewNextFoot + (newNextHead->header.block_size<<4)-8;
				sf_footer *newNextFoot = ptrToNewNextFoot;
				newNextFoot->block_size = leftOver>>4;
				newNextFoot->alloc = 0;

				sf_free_header *finder = freelist_head;
				if(finder == NULL){

					newNextHead->next = NULL;
					newNextHead->prev = NULL;
				}

				/*The freelist is not NULL, connect new free block with other free blocks*/
				else{
					if(freelist_head < newNextHead){
						while(finder < nextHead && finder->next != NULL){
							finder = finder->next;
						}
						if(finder->next == NULL && finder < nextHead){
							newNextHead->next = NULL;
							newNextHead->prev = finder;
							finder->next = newNextHead;
						}
						else{
							sf_free_header *temp = finder->prev;
							temp->next = newNextHead;
							finder->prev = newNextHead;
							newNextHead->next = finder;
							newNextHead->prev = temp;
						}
					}
					else{
						freelist_head->prev = newNextHead;
						newNextHead->next = freelist_head;
						newNextHead->prev = NULL;
						freelist_head = newNextHead;
					}
				}
			}

			/*If adjacent block is a free block, set new size for current block then coalesce with leftover*/
			else{

				/*NOTE: adjacent block now has block_size equivalent to the leftover + 16
						take nextHead->header.block_size and add that to newNextHead->header.block_size
				*/

				info_coalesces++;

				newNextHead->header.block_size = ((nextHead->header.block_size<<4) + leftOver)>>4;
				newNextHead->header.alloc = 0;
				newNextHead->header.splinter = 0;


				void *pointer = (char*)newNextHead + (newNextHead->header.block_size<<4) - 8;
				sf_footer *newNextFoot = pointer;
				newNextFoot->block_size = ((nextHead->header.block_size<<4) + leftOver)>>4;
				newNextFoot->alloc = 0;
				newNextFoot->splinter = 0;

				/*If newNextHead has address less than the freelist_head*/
				if(newNextHead < freelist_head){
					newNextHead->next = freelist_head->next;
					newNextHead->prev = freelist_head->prev;
					if(freelist_head->next != NULL){
						sf_free_header *temp = freelist_head->next;
						temp->prev = freelist_head;
					}
					freelist_head->next = NULL;
					freelist_head->prev = NULL;
					freelist_head = newNextHead;
				}
				else{
					sf_free_header *temp = nextHead;
					sf_free_header *tempo = nextHead->prev;

					temp->prev = newNextHead;
					tempo->next = newNextHead;
					newNextHead->next = nextHead->next;
					newNextHead->prev = nextHead->prev;

					nextHead->next = NULL;
					nextHead->prev = NULL;
				}
			}

			totalPayload = totalPayload - actualPayload + size;
			if(totalPayload > aggPayload)
				aggPayload = totalPayload;

			return ptr;
		}

		/*If the leftover is not big enough to be a valid free block, check if nextHead is a free block*/
		else{
			if(nextHead->header.alloc == 0){
				int oldPadding = currentHead->header.padding_size;
				int oldSplinterSize = currentHead->header.splinter_size;

				/*Set the new foot of the allocated block*/
				void *ptrToNewFoot = (char*)currentHead + newBlockSize - 8;
				sf_footer *newCurrFoot = ptrToNewFoot;


				/*Set the new head of the adjacent free block*/
				void *ptrToNewNextHead = ptrToNewFoot + 8;
				sf_free_header *newNextHead = ptrToNewNextHead;

				/*Find the foot of the next block*/
				void *oop = (char*)nextHead + (nextHead->header.block_size<<4) - 8;
				sf_footer *nextFoot = oop;

				/*Coalesce and set the contents of the block*/
				newNextHead->header.alloc = 0;
				newNextHead->header.block_size = ((nextHead->header.block_size<<4)+leftOver)>>4;
				nextFoot->block_size = ((nextHead->header.block_size<<4)+leftOver)>>4;

				newNextHead->next = nextHead->next;
				newNextHead->prev = nextHead->prev;


				/*Adjust pointers to block*/
				if(nextHead->prev != NULL){
					sf_free_header *temp = nextHead->prev;
					temp->next = newNextHead;
				}
				if(nextHead->next != NULL){
					sf_free_header *temp = nextHead->next;
					temp->prev = newNextHead;
				}
				if(nextHead == freelist_head){
					freelist_head = newNextHead;
				}

					nextHead->next = NULL;
					nextHead->prev = NULL;

				currentHead->header.block_size = newBlockSize>>4;
				currentHead->header.splinter = 0;
				currentHead->header.splinter_size = 0;
				currentHead->header.requested_size = size;
				currentHead->header.padding_size = newPad;

				newCurrFoot->block_size = newBlockSize>>4;
				newCurrFoot->splinter = 0;
				newCurrFoot->alloc = 1;

				if(oldSplinterSize != 0){
					info_splintering = info_splintering - oldSplinterSize;
					info_splinters--;
				}
				info_padding = info_padding - oldPadding + newPad;
				info_coalesces++;

			/*ONE THING I CAN DO IS USE THE CALCULATED LEFTOVER TO MOVE THE HEAD BACK AND THEN ADD THE LEFTOVE TO THE BLOCK_SIZE*/
			}

			/*If the block adjacent to what's going to be leftover is not free, we cannot coalesce, must take in splinters*/
			else{
				int temp = currentHead->header.splinter_size;
				currentHead->header.requested_size = size;
				currentHead->header.splinter = 1;
				currentHead->header.splinter_size = original_size - size - newPad - 16;
				currentHead->header.padding_size = newPad;

				currentFoot->splinter = 1;

				/*If we already had splinters*/
				if(temp != 0){
					info_splintering = info_splintering - temp + currentHead->header.splinter_size;
				}
				else{
					info_splinters++;
					info_splintering = info_splintering + currentHead->header.splinter_size;
				}
			}
			totalPayload = totalPayload - actualPayload + size;
			if(totalPayload > aggPayload)
				aggPayload = totalPayload;
			return ptr;
		}
	}


	/*If we are reallocing to a bigger size*/
	else{
		int difference = newBlockSize - (currentHead->header.block_size<<4);

		int splinter_size_info = currentHead->header.splinter_size;
		int padding_size_info = currentHead->header.padding_size;

		/*If adjacent block is free and can fit the reallocation*/
		if((nextHead->header.alloc == 0 && currentFoot != endOfHeap) && (((nextHead->header.block_size<<4) + (currentHead->header.block_size<<4)) >= (size + newPad + 16))){

			/*If the adjacent block and the current block has exactly the amount of memory needed*/
			if(((nextHead->header.block_size<<4) + (currentHead->header.block_size<<4)) == (size + newPad + 16)){
				int oldSplinterSize = currentHead->header.splinter_size;
				int oldPadding = currentHead->header.padding_size;

				int combinedSize = ((currentHead->header.block_size<<4) + (nextHead->header.block_size<<4));

				/*Adjust next/prev for freelist*/
				sf_free_header *toNext = nextHead->next;
				sf_free_header *toPrev = nextHead->prev;
				if(toNext != NULL){
					toNext->prev = toPrev;
				}
				if(toPrev != NULL){
					toPrev->next = toNext;
				}
				if(nextHead->prev == NULL){
					freelist_head = toNext;
				}
				nextHead->next = NULL;
				nextHead->prev = NULL;

				currentHead->header.block_size = combinedSize>>4;
				currentHead->header.splinter = 0;
				currentHead->header.splinter_size = 0;
				currentHead->header.requested_size = size;
				currentHead->header.padding_size = newPad;

				if(oldSplinterSize != 0){
					info_splintering = info_splintering - oldSplinterSize;
					info_splinters--;
				}else
				info_padding = info_padding - oldPadding + newPad;

				void *temp = (char*)currentHead + (currentHead->header.block_size<<4) - 8;
				sf_footer *newFoot = temp;
				newFoot->block_size = combinedSize>>4;
				newFoot->alloc = 1;
				newFoot->splinter = 0;

				void *returnAddress = ptr;
				totalPayload = totalPayload - actualPayload + size;
				if(totalPayload > aggPayload)
					aggPayload = totalPayload;
				return returnAddress;
			}



			/*If there will be no splinters*/
			if(((nextHead->header.block_size<<4) - (size - currentHeadPL)) >= 32){

				int temp = currentHead->header.splinter_size;
				int temp2 = currentHead->header.padding_size = newPad;
				/*Update the header contents*/
				currentHead->header.block_size = newBlockSize>>4;
				currentHead->header.padding_size = newPad;
				currentHead->header.splinter = 0;
				currentHead->header.splinter_size = 0;
				currentHead->header.requested_size = size;

				/*If there was a splinter before*/
				if(temp != 0){
					info_splinters--;
					info_splintering = info_splintering - temp;
				}
				else{

				}
				info_padding = info_padding - temp2 + newPad;

				/*Don't lost the pointers to next and prev!*/
				void *toNext = nextHead->next;
				void *toPrev = nextHead->prev;

				/*Update the footer contents*/
				void *ptrToFoot = (char*)currentHead + (currentHead->header.block_size<<4) - 8;
				sf_footer *newFoot = ptrToFoot;
				newFoot->block_size = newBlockSize>>4;
				newFoot->splinter = 0;
				newFoot->alloc = 1;

				void *nextFree = (char*)newFoot + 8;
				sf_free_header *newFree = nextFree;
				newFree->header.block_size = ((nextHead->header.block_size<<4) - difference)>>4;
				newFree->header.alloc = 0;
				newFree->header.splinter = 0;
				newFree->header.requested_size = 0;

				void *nextFreeFoot = (char*)newFree + (newFree->header.block_size<<4) - 8;
				sf_footer *newFreeFoot = nextFreeFoot;
				newFreeFoot->block_size = ((nextHead->header.block_size<<4) - difference)>>4;

				/*set next and prev*/
				newFree->next = toNext;
				newFree->prev = toPrev;

				if(newFree->prev != NULL){
					sf_free_header *tempor = newFree->prev;
					tempor->next = newFree;
				}
				if(newFree->next != NULL){
					sf_free_header *tempor = newFree->next;
					tempor->prev = newFree;
				}
				if(nextHead == freelist_head){
					freelist_head = newFree;
				}

				void *returnAddress = ptr;
				totalPayload = totalPayload - actualPayload + size;
				if(totalPayload > aggPayload)
					aggPayload = totalPayload;
				return returnAddress;
			}

			/*Reallocating to adjacent free block will result in a splinter*/
			else{
				int oldSplinterSize = currentHead->header.splinter_size;
				int oldPadding = currentHead->header.padding_size;

				int combinedSize = ((currentHead->header.block_size<<4) + (nextHead->header.block_size<<4));
				int theSplinter = combinedSize - (size + newPad + 16);

				/*Adjust next/prev for freelist*/
				sf_free_header *toNext = nextHead->next;
				sf_free_header *toPrev = nextHead->prev;
				if(toNext != NULL){
					toNext->prev = toPrev;
				}
				if(toPrev != NULL){
					toPrev->next = toNext;
				}
				if(nextHead->prev == NULL){
					freelist_head = toNext;
				}
				nextHead->next = NULL;
				nextHead->prev = NULL;

				currentHead->header.block_size = combinedSize>>4;
				currentHead->header.splinter = 1;
				currentHead->header.splinter_size = theSplinter;
				currentHead->header.requested_size = size;
				currentHead->header.padding_size = newPad;

				if(oldSplinterSize != 0){
					info_splintering = info_splintering - oldSplinterSize + theSplinter;
				}else{
					info_splintering = info_splintering + theSplinter;
					info_splinters++;
				}
				info_padding = info_padding - oldPadding + newPad;

				void *temp = (char*)currentHead + (currentHead->header.block_size<<4) - 8;
				sf_footer *newFoot = temp;
				newFoot->block_size = combinedSize>>4;
				newFoot->alloc = 1;
				newFoot->splinter = 1;

				void *returnAddress = ptr;
				totalPayload = totalPayload - actualPayload + size;
				if(totalPayload > aggPayload)
					aggPayload = totalPayload;
				return returnAddress;
			}
		}

		/*If adjacent block is not free or cannot hold reallocation, search free list*/
		else{

			void *returnAddress = NULL;
			while(1){

				/*Check to see if there is a free block that can hold the newBlockSize*/
				sf_free_header *finder = freelist_head;
				int found = 0;

				/*finder will iterate through free list to find a block size where reallocation will fit*/
				/*If finder equals NULL, that means there are no free blocks, must call sf_sbrk*/
				while(finder != NULL){
					if((finder->header.block_size<<4) >= newBlockSize){
						found = 1;
						break;
					}
					finder = finder->next;
				}

				/*We found a block*/
				if(found == 1){
					found = 0;
					void *dest = (char*)finder + 8;
					int original_size = finder->header.block_size<<4;

					/*Adjust prev/next for free list*/
					sf_free_header *toNext = finder->next;
					sf_free_header *toPrev = finder->prev;
					finder->header.block_size = newBlockSize>>4;

					/*set the head of the realloced block*/
					void *temp = (char*)finder + (finder->header.block_size<<4) - 8;
					sf_footer *finderFoot = temp;
					finderFoot->block_size = newBlockSize>>4;

					/*set the head of the free block*/
					temp = (char*)finderFoot + 8;
					sf_free_header *nextFree = temp;
					nextFree->header.block_size = (original_size - (finder->header.block_size<<4))>>4;

					/*set the foot of the free block*/
					temp = (char*)nextFree + (nextFree->header.block_size<<4) - 8;
					sf_footer *nextFoot = temp;
					nextFoot->block_size = (original_size - (finder->header.block_size<<4))>>4;

					nextFree->next = toNext;
					nextFree->prev = toPrev;

					/*If we will not have splinters*/
					if(original_size-newBlockSize >= 32){

						if(toNext != NULL && toNext != endOfHeap){
							toNext->prev = nextFree;
						}
						if(toPrev != NULL){
							toPrev->next = nextFree;
						}
						else{
							freelist_head = nextFree;
						}
						finder->header.alloc = 1;
						finder->header.requested_size = size;
						finder->header.splinter = 0;
						finder->header.splinter_size = 0;
						finder->header.padding_size = newPad;

						finderFoot->splinter = 0;
						finderFoot->alloc =1;

						nextFree->header.alloc = 0;
						nextFree->header.splinter = 0;
						nextFree->header.splinter_size = 0;
						nextFree->header.requested_size = 0;
						nextFree->header.padding_size = 0;

						nextFoot->splinter = 0;
						nextFoot->alloc = 0;

						/*If previous address location contained a splinter*/
						if(splinter_size_info != 0){
							info_splintering = info_splintering - splinter_size_info;
							info_splinters--;
						}
						info_padding = info_padding - padding_size_info + newPad;

						void *check = (char*)finder+8;
						returnAddress = check;
					}

					/*If our reallocation will have splinters*/
					else{
						/*set next/prev*/
						if(toNext != NULL){
							toNext->prev = toPrev;
						}
						if(toPrev != NULL){
							toPrev->next = toNext;
						}
						if(toPrev == NULL){
							freelist_head = toNext;
						}

						finder->header.block_size = original_size>>4;
						finder->header.splinter = 1;
						finder->header.alloc = 1;
						finder->header.requested_size = size;
						finder->header.splinter_size = (original_size - newBlockSize);
						finder->header.padding_size = newPad;

						void *temp = (char*)finder + (finder->header.block_size<<4) - 8;
						finderFoot = temp;
						finderFoot->block_size = original_size >> 4;
						finderFoot->splinter = 1;
						finderFoot->alloc = 1;

						/*If we had a splinter before reallocation*/
						if(splinter_size_info != 0){
							info_splintering = info_splintering - splinter_size_info + finder->header.splinter_size;
						}
						else{
							info_splinters++;
							info_splintering = info_splintering + finder->header.requested_size;
						}
						info_padding = info_padding - padding_size_info + newPad;

						temp = finder;
						returnAddress = (char*)temp+8;
					}

					/*Transfer the information*/
					memcpy(dest, ptr, size);
					sf_free(ptr);
					totalPayload = totalPayload - actualPayload + size;
					if(totalPayload > aggPayload)
						aggPayload = totalPayload;
					return returnAddress;
				}

				/*No block in the free list, must call sf_sbrk*/
				else{

					/*Call sf_sbrk*/

					void *newPage = sf_sbrk(4096);
					max--;
					++i;
					if(max < 0){
						printf("ERROR: Memory overflow: sf_sbrk has already been called 4 times!\n");
						errno = ENOMEM;
						return (void*) -1;
					}

					sf_free_header *pageHead = newPage;
					pageHead->header.block_size = 4096 >> 4;
					pageHead->header.alloc = 0;
					pageHead->header.splinter = 0;
					pageHead->header.requested_size = 0;
					pageHead->header.padding_size = 0;
					pageHead->header.splinter_size = 0;
					pageHead->next = NULL;
					pageHead->prev = NULL;

					/*newPage foot*/
					void *newPageFoot = (char*)pageHead + 4096 - 8;
					endOfHeap = newPageFoot;
					sf_footer *pageFoot = newPageFoot;
					pageFoot->block_size = 4096 >> 4;
					pageFoot-> alloc = 0;
					pageFoot->splinter = 0;

					void *beforeNewPage = (char*)pageHead-8;
					sf_footer *prevFoot = beforeNewPage;
					if(prevFoot->alloc == 0){

						int combinedSize = (prevFoot->block_size<<4) + 4096;

						void *temp = (char*)prevFoot - (prevFoot->block_size<<4) + 8;
						sf_free_header *prevHead = temp;
						prevHead->header.block_size = combinedSize >> 4;
						pageFoot->block_size = combinedSize >> 4;
						pageFoot->alloc = 0;
						pageFoot->splinter = 0;

						info_coalesces++;
					}

					/*If previous adjacent block is allocated, set next/prev*/
					else{

						sf_free_header *temp = freelist_head;
						if(temp == NULL){
							freelist_head = temp;
							temp->next = NULL;
							temp->prev = NULL;
						}
						else{
							while(temp->next != NULL){
								temp = temp->next;
							}
							temp->next = pageHead;
							pageHead->prev = temp;
						}
					}
				}
			}
		}
	}
	return NULL;
}



//ignore: PAGE TAB 333
void sf_free(void* ptrToFree) {

	sf_free_header *theHead = ptrToFree-8;


	/*set pointer to point to the foot of the current block*/
	void *pointer = (char*)ptrToFree+(theHead->header.block_size<<4)-16;
	sf_footer *theFoot = pointer;


	/*Check the given pointer if valid*/
	if(ptrToFree < startOfHeap || ptrToFree > endOfHeap){
		printf("ERROR: ptrToFree not in heap\n");
		errno = EINVAL;
		return;
	}

	if(theHead->header.alloc != 1 || theFoot->alloc != 1){
		printf("ERROR: Alloc bit(s) are set to 1!\n");
		errno = EINVAL;
		return;
	}
	if(theHead->header.splinter != theFoot->splinter){
		printf("ERROR: Splinter bits are not the same!\n");
		errno = EINVAL;
		return;
	}
	if((theHead->header.splinter == 1 && theHead->header.splinter_size == 0) || (theFoot->splinter == 1 && theHead->header.splinter_size == 0)){
		printf("ERROR: Splinter bit set to 1 but splinter_size is 0!\n");
		errno = EINVAL;
		return;
	}
	if(theHead->header.block_size != theFoot->block_size){
		printf("ERROR: Block_size not the same!\n");
		errno = EINVAL;
		return;
	}

	theHead->header.alloc = 0;
	theFoot->alloc = 0;

	/*COALESCE*/
	/*set prevFoot to point to the foot of the previous block*/
	void *pointer2 = ptrToFree-16;
	sf_footer *prevFoot = pointer2;

	/*set prevHead to point to the head of the previous block*/
	void *pointer3 = (char*)pointer2-(prevFoot->block_size<<4)+8;
	sf_free_header *prevHead = pointer3;

	/*set nextHead to point to the head of the next block*/
	sf_free_header *nextHead = pointer + 8;

	/*set nextFoot to point to the foot of the next block*/
	void *pointer4 = (char*)nextHead + (nextHead->header.block_size<<4)-8;
	sf_footer *nextFoot = pointer4;



	info_alloc--;
	if(theHead->header.splinter == 1){
		info_splintering = info_splintering - theHead->header.splinter_size;
		info_splinters--;
	}
	info_padding = info_padding - theHead->header.padding_size;
	int actualPayload = (theHead->header.block_size<<4) - theHead->header.padding_size - theHead->header.splinter_size - 16;
	totalPayload = totalPayload - actualPayload;

	/*Check if we are at the boundaries of the heap*/
	int leftIsClear = 1;
	int rightIsClear = 1;
	if(startOfHeap == ptrToFree-8){
		leftIsClear = 0;
	}
	if(endOfHeap == theFoot){
		rightIsClear = 0;
	}


	/*If both sides of the block are allocated*/
	if(prevFoot->alloc == 1 && nextHead->header.alloc == 1){
		if(freelist_head == NULL){
			freelist_head = theHead;
			theHead->next = NULL;
			theHead->prev = NULL;
			return;
		}
		else if(theHead < freelist_head){

			theHead->next = freelist_head;
			freelist_head->prev = theHead;
			freelist_head = theHead;
		}
		else{
			sf_free_header *newNextHead = freelist_head;
			while(newNextHead < theHead && newNextHead->next != NULL){
				newNextHead = newNextHead->next;
			}
			if(newNextHead != NULL){
				sf_free_header *newPrevHead = newNextHead->prev;
				newNextHead->prev = theHead;
				newPrevHead->next = theHead;
				theHead->next = newNextHead;
				theHead->prev = newPrevHead;
			}
			else{
				newNextHead->next = theHead;
				theHead->prev = newNextHead;
				theHead->next = NULL;
			}
		}


	}

	/*If right side is free*/
	else if(prevFoot->alloc == 1 && nextHead->header.alloc == 0){
		if(freelist_head == NULL){
			freelist_head = theHead;
			theHead->next = NULL;
			theHead->prev = NULL;
			return;
		}
		if(rightIsClear==1){

			info_coalesces++;
			int combinedSize = (theHead->header.block_size<<4) + (nextHead->header.block_size<<4);
			if(freelist_head < theHead){
				sf_free_header *runner = freelist_head;

				/*Since there is a freeblock to the right of theHead, we are certain that there WILL be a valid free block
					right after theHead
				*/
				while(runner < theHead){
					runner = runner->next;
				}
				/*runner is now the block right next to theHead*/

				if(runner->next != NULL){
					sf_free_header *afterRunner = runner->next;
					afterRunner->prev = theHead;
					theHead->next = afterRunner;
				}
				else{
					theHead->next = NULL;
				}
				runner = runner->prev;
				runner->next = theHead;
				theHead->prev = runner;

				theHead->header.block_size = combinedSize>>4;
				theHead->header.alloc = 0;
				nextFoot->block_size = combinedSize>>4;
				nextFoot->alloc = 0;
			}
			else{
				theHead->next = freelist_head->next;
				theHead->prev = freelist_head->prev;

				if(freelist_head->next != NULL){
					sf_free_header *theAfter = freelist_head->next;
					theAfter->prev = theHead;
				}

				freelist_head = theHead;
				theHead->header.block_size = combinedSize>>4;
				theHead->header.alloc = 0;
				nextFoot->block_size = combinedSize>>4;
				nextFoot->alloc = 0;
			}

			nextHead->prev = NULL;
			nextHead->next = NULL;
		}
		else{
			if(freelist_head != NULL){
				sf_free_header *connector = freelist_head;
				sf_free_header *connector2 = connector;
				while(connector != NULL){
					connector2 = connector;
					connector = connector->next;
				}
			connector2->next = theHead;
			theHead->prev = connector2;
			theHead->next = NULL;
			}
			else{
				freelist_head = theHead;
				freelist_head->prev = NULL;
				freelist_head->next = NULL;
			}

		}

	}

	/*Coalescing to the left*/
	else if(prevFoot->alloc == 0 && nextHead->header.alloc == 1){
		if(freelist_head == NULL){
			freelist_head = theHead;
			theHead->next = NULL;
			theHead->prev = NULL;
			return;
		}
			if(startOfHeap == ptrToFree-8){

				/*set the next and prev*/
				freelist_head->prev = theHead;
				theHead->next = freelist_head;
				theHead->prev = NULL;
				freelist_head = theHead;

				return;
			}
			else{
			}
			info_coalesces++;
			int combinedSize = (theHead->header.block_size<<4) + (prevHead->header.block_size<<4);
			prevHead->header.block_size = combinedSize>>4;
			theFoot->block_size = combinedSize>>4;

			theHead->next = NULL;
			theHead->prev = NULL;

			freelist_head = prevHead;

	}

	/*Coalescing on both sides*/
	else if(prevFoot->alloc == 0 && nextHead->header.alloc == 0){


		/*Coalescing not possible*/
		if(leftIsClear == 0 && rightIsClear == 0){
			if(freelist_head == NULL){
				freelist_head = theHead;
				theHead->next = NULL;
				theHead->prev = NULL;
			}
			return;
		}
		/*Coalescing only possible with next block*/
		else if(leftIsClear == 0 && rightIsClear == 1){
			info_coalesces++;
			int combinedSize = (theHead->header.block_size<<4) + (nextHead->header.block_size<<4);

			theHead->header.block_size = combinedSize>>4;
			nextFoot->block_size = combinedSize>>4;
			if(nextHead->next != NULL){
				sf_free_header *theNext = nextHead->next;
				theNext->prev = theHead;
				theHead->next = theNext;
			}
			else{
				theHead->next = NULL;
				theHead->prev = NULL;
			}

			nextHead->prev = NULL;
			nextHead->next = NULL;

			if(theHead < freelist_head){
				freelist_head = theHead;
				freelist_head->prev = NULL;
			}
		}

		/*Coalescing only possible with previous block*/
		else if(leftIsClear == 1 && rightIsClear == 0){
			info_coalesces++;

			int combinedSize = (theHead->header.block_size<<4) + (prevHead->header.block_size<<4);
			prevHead->header.block_size = combinedSize>>4;
			theFoot->block_size = combinedSize>>4;
			theHead->next = NULL;
			theHead->prev = NULL;
		}

		/*Coalescing possible with both blocks*/
		else{
			info_coalesces++;
			int combinedSize = (prevHead->header.block_size<<4) + (theHead->header.block_size<<4) + (nextHead->header.block_size<<4);
			prevHead->header.block_size = combinedSize>>4;
			nextFoot->block_size = combinedSize>>4;

			if(nextHead->next != NULL){
				sf_free_header *farEast = nextHead->next;
				prevHead->next = farEast;
				farEast->prev = prevHead;
			}
			else{
				prevHead->next = NULL;
			}

			theHead->next = NULL;
			theHead->prev = NULL;
			nextHead->next = NULL;
			nextHead->prev = NULL;
		}
	}
	return;
}

int sf_info(info* ptr) {
	ptr->allocatedBlocks = info_alloc;
	ptr->splinterBlocks = info_splinters;
	ptr->padding = info_padding;
	ptr->splintering = info_splintering;
	ptr->coalesces = info_coalesces;
	ptr->peakMemoryUtilization = aggPayload/(4096 * i);
	printf("allocatedBlocks: %lu\n", ptr->allocatedBlocks);
	printf("splinterBlocks: %lu\n", ptr->splinterBlocks);
	printf("padding: %lu\n", ptr->padding);
	printf("splintering: %lu\n", ptr->splintering);
	printf("coalesces: %lu\n", ptr->coalesces);
	printf("peakMemoryUtilization: %f\n", ptr->peakMemoryUtilization);
	return 0;
}
