#include <criterion/criterion.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sfmm.h"

/**
 *  HERE ARE OUR TEST CASES NOT ALL SHOULD BE GIVEN STUDENTS
 *  REMINDER MAX ALLOCATIONS MAY NOT EXCEED 4 * 4096 or 16384 or 128KB
 */

Test(sf_memsuite, Malloc_an_Integer, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(sizeof(int));
  *x = 4;
  cr_assert(*x == 4, "Failed to properly sf_malloc space for an integer!");
}

Test(sf_memsuite, Free_block_check_header_footer_values, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *pointer = sf_malloc(sizeof(short));
  sf_free(pointer);
  pointer = (char*)pointer - 8;
  sf_header *sfHeader = (sf_header *) pointer;
  cr_assert(sfHeader->alloc == 0, "Alloc bit in header is not 0!\n");
  sf_footer *sfFooter = (sf_footer *) ((char*)pointer + (sfHeader->block_size << 4) - 8);
  cr_assert(sfFooter->alloc == 0, "Alloc bit in the footer is not 0!\n");
}

Test(sf_memsuite, SplinterSize_Check_char, .init = sf_mem_init, .fini = sf_mem_fini){
  void* x = sf_malloc(32);
  void* y = sf_malloc(32);
  (void)y;

  sf_free(x);

  x = sf_malloc(16);

  sf_header *sfHeader = (sf_header *)((char*)x - 8);
  cr_assert(sfHeader->splinter == 1, "Splinter bit in header is not 1!");
  cr_assert(sfHeader->splinter_size == 16, "Splinter size is not 16");

  sf_footer *sfFooter = (sf_footer *)((char*)sfHeader + (sfHeader->block_size << 4) - 8);
  cr_assert(sfFooter->splinter == 1, "Splinter bit in header is not 1!");
}

Test(sf_memsuite, Check_next_prev_pointers_of_free_block_at_head_of_list, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  memset(x, 0, 0);
  cr_assert(freelist_head->next == NULL);
  cr_assert(freelist_head->prev == NULL);
}

Test(sf_memsuite, Coalesce_no_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(4);
  int *y = sf_malloc(4);
  memset(y, 0, 0);
  sf_free(x);

  //just simply checking there are more than two things in list
  //and that they point to each other
  cr_assert(freelist_head->next != NULL);
  cr_assert(freelist_head->next->prev != NULL);
}

//#
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE THESE COMMENTS
//#






/*Testing sf_malloc by mallocing the entire page*/

Test(sf_memsuite, Malloc_enire_block, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *x = sf_malloc(4080);
  void *temp = (char*)x - 8;
  sf_free_header *theHead = temp;
  temp = (char*)theHead + (theHead->header.block_size<<4) - 8;
  sf_footer *theFoot = temp;

  cr_assert(freelist_head == NULL, "freelist_head is not NULL!");

  cr_assert(theHead->header.alloc == 1, "The header alloc bit is not 1!");
  cr_assert(theHead->header.splinter == 0, "The header splinter bit is not 0!");
  cr_assert(theHead->header.block_size<<4 == 4096, "The block size is not 4096!");
  cr_assert(theHead->header.requested_size == 4080, "The requested size is not 4080!");
  cr_assert(theHead->header.splinter_size == 0, "The splinter size is not 0!");
  cr_assert(theHead->header.padding_size == 0, "The padding size is not 0!");

  cr_assert(theFoot->alloc == 1, "The footer alloc bit is not 1!");
  cr_assert(theFoot->splinter == 0, "The footer splinter bit is not 0!");
  cr_assert(theFoot->block_size<<4 == 4096, "The block size is not 4096!");
}


/*Test calling for additional memory using sf_sbrk by allocating a large amount of memory*/
/*Test for Case 3 coalescing*/

Test(sf_memsuite, Malloc_large_amount_of_memory, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(12272);

  /*Check if the header contents are correct after sf_mallocing 12272 bytes*/
  sf_header *sfHeader = (sf_header *)((char*)x - 8);
  cr_assert(sfHeader->alloc == 1, "Alloc bit is not 1!");
  cr_assert(sfHeader->splinter == 0, "Splinter bit is not 0!");
  cr_assert(sfHeader->block_size<<4 == 12288, "Block size is not 12016!");
  cr_assert(sfHeader->requested_size == 12272, "Requested size is not 12000!");
  cr_assert(sfHeader->splinter_size == 0, "Splinter size is not 0!");
  cr_assert(sfHeader->padding_size == 0, "Padding size is not 0!");

  /*Check if the footer contents are correct after sf_mallocing 12272 bytes*/
  sf_footer *sfFooter = (sf_footer *)((char*)sfHeader + (sfHeader->block_size << 4) - 8);
  cr_assert(sfFooter->alloc == 1, "Alloc bit is not 1!");
  cr_assert(sfFooter->splinter == 0, "Splinter bit is not 0!");
  cr_assert(sfFooter->block_size<<4 == 12288, "Block size is not 12016!");
}




/*Test calling for additional memory using sf_sbrk by allocating a large amount of memory and then freeing*/
/*Test for Case 3 coalescing*/
/*Test for Case 2 coalescing*/

Test(sf_memsuite, Malloc_large_amount_of_memory_and_free, .init = sf_mem_init, .fini = sf_mem_fini) {
  int *x = sf_malloc(12000);

  /*Check if the header contents are correct after sf_mallocing 12000 bytes*/
  sf_header *sfHeader = (sf_header *)((char*)x - 8);
  cr_assert(sfHeader->alloc == 1, "Alloc bit is not 1!");
  cr_assert(sfHeader->splinter == 0, "Splinter bit is not 0!");
  cr_assert(sfHeader->block_size<<4 == 12016, "Block size is not 12016!");
  cr_assert(sfHeader->requested_size == 12000, "Requested size is not 12000!");
  cr_assert(sfHeader->splinter_size == 0, "Splinter size is not 0!");
  cr_assert(sfHeader->padding_size == 0, "Padding size is not 0!");

  /*Check if the footer contents are correct after sf_mallocing 12000 bytes*/
  sf_footer *sfFooter = (sf_footer *)((char*)sfHeader + (sfHeader->block_size << 4) - 8);
  cr_assert(sfFooter->alloc == 1, "Alloc bit is not 1!");
  cr_assert(sfFooter->splinter == 0, "Splinter bit is not 0!");
  cr_assert(sfFooter->block_size<<4 == 12016, "Block size is not 12016!");

  /*sf_free the sf_malloced memory*/
  sf_free(x);

  /*Check to see if the block_size and the alloc bit are properly set*/
  /*new_sfFooter variable declared because we are expecting coalescing*/
  /*Block_size should be 12288 because not the entirety of the 3 pages are sf_malloced, so coalescing occurred after sf_freeing*/
  cr_assert(sfHeader->block_size<<4 == 12288, "Block_size is not 12288!");
  cr_assert(sfHeader->alloc == 0, "Alloc bit is not 0!");

  sf_footer *new_sfFooter = (sf_footer *)((char*)sfHeader + (sfHeader->block_size << 4) - 8);
  cr_assert(new_sfFooter->block_size<<4 == 12288, "Block_size is not 12288!");
  cr_assert(new_sfFooter->alloc == 0, "Alloc bit is not 0!");
}




/*Test for Case 1 coalescing*/
/*Test for Case 2 coalescing*/
/*Test for Case 4 coalescing*/

Test(sf_memsuite, Case_4_coalescing, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *a = sf_malloc(48);
  void *b = sf_malloc(48);
  void *c = sf_malloc(48);
  void *d = sf_malloc(48);
  void *e = sf_malloc(48);

  /*Test if the freelist consists of only one block*/
  cr_assert(freelist_head->next == NULL);
  cr_assert(freelist_head->prev == NULL);


  /*Case 1 coalescing (with startOfHeap check)*/
  sf_free(a);
  /*Case 1 coalescing*/
  sf_free(c);
  /*Case 2 coalescing*/
  sf_free(e);


  /*Set values for comparison*/
  void *temp = (char*)a-8;
  sf_free_header *temp1 = temp;

  temp = (char*)c-8;
  sf_free_header *temp2 = temp;

  temp = (char*)e-8;
  sf_free_header *temp3 = temp;

  /*Check the next for each free block*/
  cr_assert(temp1->next == temp2, "a->next does not point to c!");
  cr_assert(temp2->next == temp3, "c->next does not point to e!");
  cr_assert(temp3->next == NULL, "e->next does not point to NULL!");

  /*Check the prev for each free block*/
  cr_assert(temp1->prev == NULL, "a->prev does not point to NULL!");
  cr_assert(temp2->prev == temp1, "c->next does not point to a!");
  cr_assert(temp3->prev == temp2, "e->prev does not point to c!");

  //For visualizing
  sf_snapshot(true);
  //sf_varprint(a);
  //sf_varprint(b);
  //sf_varprint(c);
  //sf_varprint(d);
  //sf_varprint(e);


  /*Case 4 coalescing*/
  sf_free(b);
  cr_assert(temp1->next == temp3, "a->next does not point to e!");

  /*Case 4 coalescing (at this point the entire heap should be free)*/
  sf_free(d);
  cr_assert(temp1->next == NULL, "a->next does not point to NULL!");
  cr_assert(temp1->prev == NULL, "a->prev does not point to NULL!");

  //For visualizing
  //sf_snapshot(true);
  //sf_varprint(a);
  //sf_varprint(b);
  //sf_varprint(c);
  //sf_varprint(d);
  //sf_varprint(e);
}


/*Testing sf_realloc to smaller size with block splitting*/

Test(sf_memsuite, Realloc_with_splitting, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *x = sf_malloc(64);
  void *y = sf_malloc(64);
  sf_realloc(x,16);

  /*Is the freelist_head after the foot of the reallocated block?*/
  void *temp = (char*)x + 24;
  sf_free_header *freeListHead = temp;
  cr_assert(freeListHead == freelist_head, "freelist_head is at the wrong address!");

  /*Is freelist_head->next after the foot of y?*/
  temp = (char*)y + 72;
  cr_assert(temp == freelist_head->next, "freelist_head->next points to the wrong address!");

  /*4096 - 80 (bytes from x and the freed split block) - 80 (bytes from y) is equal to 3936 bytes*/
  sf_free_header *nextFreeBlock = temp;
  cr_assert((nextFreeBlock->header.block_size<<4) == 3936, "Block size after y is incorrect!");
}


/*Testing sf_realloc to smaller size with splinters*/

Test(sf_memsuite, Realloc_with_splinters, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *x = sf_malloc(64);
  void *y = sf_malloc(63);
  sf_realloc(x,48);

  void *temp = (char*)x - 8;
  sf_free_header *firstBlock = temp;
  temp = (char*)y - 8;
  sf_free_header *secondBlock = temp;
  temp = (char*)y - 16;
  sf_footer *firstBlockFoot = temp;
  temp = (char*)secondBlock + (secondBlock->header.block_size<<4) - 8;
  sf_footer *secondBlockFoot = temp;
  temp = (char*)temp + 8;
  sf_free_header *freeBlock = temp;

  cr_assert(firstBlock->header.alloc == 1, "alloc bit is not 1!");
  cr_assert(firstBlock->header.splinter == 1, "Splinter bit is not 1!");
  cr_assert(firstBlock->header.block_size<<4 == 80, "Block size is not 80!");
  cr_assert(firstBlock->header.requested_size == 48, "SRequested size is not 48!");
  cr_assert(firstBlock->header.padding_size == 0, "Padding size is not 0!");
  cr_assert(firstBlock->header.splinter_size == 16, "Splinter size is not 16!");

  cr_assert(firstBlockFoot->alloc = 1, "alloc bit is not 1!");
  cr_assert(firstBlockFoot->splinter == 1, "Splinter bit is not 1!");
  cr_assert(firstBlockFoot->block_size<<4 == 80, "Block size is not 80!");

  cr_assert(secondBlock->header.alloc == 1, "alloc bit is not 1!");
  cr_assert(secondBlock->header.splinter == 0, "Splinter bit is not 0!");
  cr_assert(secondBlock->header.block_size<<4 == 80, "Block size is not 80!");
  cr_assert(secondBlock->header.requested_size == 63, "Requested size is not 63!");
  cr_assert(secondBlock->header.padding_size == 1, "Padding size is not 1!");
  cr_assert(secondBlock->header.splinter_size == 0, "Splinter size is not 0!");

  cr_assert(secondBlockFoot->alloc = 1, "alloc bit is not 1!");
  cr_assert(secondBlockFoot->splinter == 0, "Splinter bit is not 0!");
  cr_assert(secondBlockFoot->block_size<<4 == 80, "Block size is not 80!");

  cr_assert(freelist_head != NULL, "freelist_head is NULL!");
  cr_assert(freelist_head == freeBlock, "freelist_head is at the wrong address!");
  cr_assert(freeBlock->header.block_size<<4 == 3936, "Free block size does not equal to 3936!");
  cr_assert(freeBlock->next == NULL, "freeBlock->head is not NULL");
  cr_assert(freeBlock->prev == NULL, "freeBlock->prev is not NULL");
}




/*Testing sf_realloc with a large amount of requested size*/

Test(sf_memsuite, Realloc_with_massive_size, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *x = sf_malloc(64);
  void *y = sf_malloc(64);

  void *check = (char*)x - 8;

  sf_free_header *abandonedBlock = check;
  sf_realloc(x,12000);

  /*Was the block freed correctly?*/
  cr_assert(abandonedBlock->header.alloc == 0, "abandonedBlock is not free!");
  cr_assert((abandonedBlock->header.block_size<<4) == 80, "Block size of x is incorrect!");
  cr_assert(abandonedBlock == freelist_head, "abandonedBlock is not the freelist_head!");

  void *temp = (char*)y+72;
  sf_free_header *xNow = temp;

  /*Has x been reallocated with the correct size?*/
  cr_assert((xNow->header.block_size<<4) == 12016, "x has been reallocated with the wrong size!");
  cr_assert(xNow->header.padding_size == 0, "padding size is incorrect!");
  cr_assert(xNow->header.alloc == 1, "alloc bit is incorrect!");
  cr_assert(xNow->header.requested_size == 12000, "requested size is incorrect!");
  cr_assert(xNow->header.splinter == 0, "splinter bit is incorrect!");
  cr_assert(xNow->header.splinter_size == 0, "splinter size is incorrect!");

  temp = (char*)xNow + (xNow->header.block_size<<4);
  sf_free_header *freeBlock = temp;

  cr_assert(freeBlock->header.alloc == 0, "The alloc bit of the last free block is not 0!");
  cr_assert(freeBlock->header.block_size<<4 == 112, "The block size of the last free block is not 112!");
  cr_assert(abandonedBlock->next == freeBlock, "freelist_head->next points to the wrong address!");
}


/*Testing realloc with large request and splinter handling*/
Test(sf_memsuite, Realloc_with_massive_size_with_splinters, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *x = sf_malloc(64);
  void *y = sf_malloc(64);

  void *check = (char*)x - 8;

  sf_free_header *abandonedBlock = check;
  sf_realloc(x,3904);

  cr_assert((abandonedBlock->header.block_size<<4) == 80, "Block size of x is incorrect!");
  cr_assert(abandonedBlock == freelist_head, "abandonedBlock is not the freelist_head!");

  void *temp = (char*)y+72;
  sf_free_header *xNow = temp;

  /*Has x been reallocated with the correct size?*/
  cr_assert((xNow->header.block_size<<4) == 3936, "x has been reallocated with the wrong size!");
  cr_assert(xNow->header.alloc == 1, "alloc bit is not 1!");
  cr_assert(xNow->header.splinter == 1, "splinter bit is not 1!");
  cr_assert(xNow->header.splinter_size == 16, "splinter_size is not 16!");
  cr_assert(xNow->header.requested_size == 3904, "requested_size is not 3904!");

  temp = (char*)xNow + (xNow->header.block_size<<4) - 8;
  sf_footer *xNowFoot = temp;

  cr_assert(xNowFoot->alloc == 1, "alloc bit is not 1!");
  cr_assert(xNowFoot->splinter == 1, "splinter bit is not 1!");
  cr_assert(xNowFoot->block_size<<4 == 3936, "block size is not 3936!");

  cr_assert(freelist_head == abandonedBlock, "abandonedBlock is not the freelist_head!");
}


/*Testing sf_realloc with a large amount of requested size and then freeing*/
Test(sf_memsuite, Malloc_Realloc_Free, .init = sf_mem_init, .fini = sf_mem_fini) {
  void *x = sf_malloc(64);
  void *y = sf_malloc(64);

  void *check = (char*)x - 8;

  sf_free_header *abandonedBlock = check;
  void *new = sf_realloc(x,12000);

  sf_free(new);

  cr_assert((abandonedBlock->header.block_size<<4) == 80, "Block size of x is incorrect!");
  cr_assert(abandonedBlock == freelist_head, "abandonedBlock is not the freelist_head!");

  void *temp = (char*)y+72;
  sf_free_header *xNow = temp;

  /*Has x been freed?*/
  cr_assert((xNow->header.block_size<<4) == 12128, "Block size of freed realloced block is incorrect!");
  cr_assert((xNow->header.alloc) == 0, "alloc bit of freed realloced bit is not 0!");

  temp = (char*)xNow + (xNow->header.block_size<<4) - 8;
  sf_footer *xFoot = temp;

  cr_assert(xFoot->block_size<<4 == 12128, "Block size of freed realloced block is incorrect!");
  cr_assert(xFoot->alloc == 0, "alloc bit of freed realloced block is not 0!");
}


/*Testing sf_info*/
Test(sf_memsuite, sf_info, .init = sf_mem_init, .fini = sf_mem_fini) {


  void *x = sf_malloc(64);
  void *y = sf_malloc(64);
  void *z = sf_malloc(65);
  void *zz = sf_malloc(65);
  printf("z is at %p and zz is at %p\n", z, zz);

  sf_free(y);
  void *newX = sf_realloc(x, 128);
  printf("newX is at %p\n", newX);

  printf("\n\n--Program Statistics--\n\n");
  printf("sizeof(info) = %lu\n", sizeof(info));
  info *ptr = sf_malloc(sizeof(info));

  /*
  allocatedBlocks: newX, z, zz, ptr
  splinterBlocks: newX
  splintering: newX(16)
  padding: z(15), zz(15)
  coalesces: None
  */

  sf_info(ptr);

  cr_assert(ptr->allocatedBlocks == 4, "Number of allocated blocks is not 4!");
  cr_assert(ptr->splinterBlocks == 1, "Number of blocks with splinters is not 1!");
  cr_assert(ptr->splintering == 16, "Size of splintering is not 16!");
  cr_assert(ptr->padding == 30, "Padding size is not 30!");
  cr_assert(ptr->coalesces == 0, "There should be no coalescing!");
}

/*Peak mem test*/
Test(sf_memsuite, sf_info_peak_mem, .init = sf_mem_init, .fini = sf_mem_fini) {


  info *x = sf_malloc(2048);
  void *y = sf_malloc(1024);
  void *z = sf_malloc(2048);
  printf("y = %p\n z = %p\n", y, z);

  sf_info(x);

  sf_realloc(z, 512);

  sf_info(x);
}

/*Testing realloc to smaller size with splinters*/
Test(sf_memsuite, realloc_to_smaller_with_splinters, .init = sf_mem_init, .fini = sf_mem_fini) {

  void *x = sf_malloc(64);
  void *y = sf_malloc(64);
  sf_realloc(x, 48);
  printf("y is at %p\n", y);

  void *p = (char*) x - 8;
  sf_free_header *xx = p;

  cr_assert(xx->header.block_size<<4 == 80, "incorrect block size!");
  cr_assert(xx->header.alloc == 1, "alloc bit is incorrect!");
  cr_assert(xx->header.splinter == 1, "splinter bit is incorrect!");
  cr_assert(xx->header.splinter_size == 16, "splinter_size is incorrect!");
  cr_assert(xx->header.requested_size == 48, "requested_size is incorrect!");
  cr_assert(xx->header.padding_size == 0, "padding_size is incorrect!");

  void *f = (char*)xx + (xx->header.block_size<<4) - 8;
  sf_footer *ff = f;

  cr_assert(ff->block_size<<4 == 80, "block_size is incorrect!");
  cr_assert(ff->splinter == 1, "splinter bit is incorrect!");
  cr_assert(ff->alloc == 1, "alloc bit is incorrect!");
}

