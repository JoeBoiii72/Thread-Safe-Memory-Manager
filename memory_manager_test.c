/*
*----------------------------------------------------------------------------*
*  memory_manager_test.c                                                              *
*                                                                            *
*  Author: Joe Kenyon                                                        *
*                                                                            *
*  Last Updated: 06/12/2019                                                  *
*                                                                            *
*  Description: Perfroms various tests on the thread-safe memory             *
*               manager implemented in part3.c                               *
*                                                                            *
*               For every algortithm we will run a number of threads         *
*               and simultaneously run soak and merg tests, this will        *
*               ensure that thread synchronization is taking place.          *
*               We can use the validate function to ensure everything        *
*               is correct and are memory manager out not broken.            *
*----------------------------------------------------------------------------*
*/



#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <pthread.h>

#include "memory_manager.h"


// allocate the memory for the allocater
#define NUMBER_OF_BLOCKS 1000
#define THREAD_NUMBER 150
#define MEMORY_SIZE 10000
static uint8_t memory_buffer[MEMORY_SIZE];


/*------------------------------------------------------*/


// generate a random number between two values
static int random_num(int start, int end)
{
  int r = rand();
  return start + (r % end);
}


/*------------------------------------------------------*/


// this tests out the merging of free blocks
static void merg_test()
{ 
  void* blocks[NUMBER_OF_BLOCKS];
  memset(blocks, 0, sizeof(blocks));

  // allocate as many block as we can
  for (int n = 0; n < NUMBER_OF_BLOCKS; n++)
  {
    blocks[n] = allocate(64);
  }
  validate();

  // deallocate every odd block
  for (int n = 0; n < NUMBER_OF_BLOCKS; n += 2)
  {
    deallocate(blocks[n]);
	blocks[n] = NULL;
  }
  validate();

  // now deallocate everything left
  // they should merg together
  for (int n = 0; n < NUMBER_OF_BLOCKS; n++)
  {
    deallocate(blocks[n]);
  }

  // if it passes are validate functions
  // nothing has gone wrong
  validate();
}



/*------------------------------------------------------*/


// this just does a soak test
static void soak_test()
{
  void* blocks[NUMBER_OF_BLOCKS];
  memset(blocks, 0, sizeof(blocks));

  for (int n = 0; n < 2500; n++)
  {
    // randomly allocate some blocks of random size
    int block = random_num(0, NUMBER_OF_BLOCKS);
    int size = random_num(1, 2046);

    if (blocks[block] == NULL)
    {
      blocks[block] = allocate(size);
    }
    else
    {
      deallocate(blocks[block]);
      blocks[block] = NULL;
    }
  }
  validate();

  // now cleanup
  for (int n = 0; n < NUMBER_OF_BLOCKS; n++)
  {
    deallocate(blocks[n]);
  }
  validate();
}


/*------------------------------------------------------*/


static void* run_tests(void* arg)
{
  soak_test();
  merg_test();
}


/*------------------------------------------------------*/

// creates a number of threads
// every thread will run a soak and merg test then stop
static void start_test_threads()
{
  pthread_t tid[THREAD_NUMBER];

  for (int i = 0; i < THREAD_NUMBER; i++)
  {
      if(pthread_create(&(tid[i]), NULL, &run_tests, NULL) != 0)
        tid[i] = (pthread_t)NULL;
  }

  for (int i = 0; i < THREAD_NUMBER; i++)
  {
    if(tid[i])
      pthread_join(tid[i], NULL);
  }
}


/*------------------------------------------------------*/


static void test_first_fit()
{
  printf("FIRSTFIT TEST\n");
  for(int i = 0; i < 5; i ++)
  {
    printf("[*] Running soak & merg tests on %d threads...\n",THREAD_NUMBER);
    initialise(memory_buffer, MEMORY_SIZE, FIRSTFIT);
    start_test_threads();
    printf("[!] SOAK & MERG TESTS PASSED\n");

    // validate our memory manager
    validate();
  }
  
  // we should now be left with one free node
  print_all_nodes();
  printf("========================\n");
}


/*------------------------------------------------------*/


static void test_next_fit()
{
  printf("NEXTFIT TEST\n");
  for(int i = 0; i < 5; i ++)
  { 
    printf("[*] Running soak & merg tests on %d threads...\n",THREAD_NUMBER);
    initialise(memory_buffer, MEMORY_SIZE, NEXTFIT);
    start_test_threads();
    printf("[!] SOAK & MERG TESTS PASSED\n");

    // validate our memory manager
    validate();
  }
  
  // we should now be left with one free node
  print_all_nodes();
  printf("========================\n");
}


/*------------------------------------------------------*/


static void test_best_fit()
{
  printf("BESTFIT TEST\n");
  for(int i = 0; i < 5; i ++)
  {
    printf("[*] Running soak & merg tests on %d threads...\n",THREAD_NUMBER);
    initialise(memory_buffer, MEMORY_SIZE, BESTFIT);
    start_test_threads();
    printf("[!] SOAK & MERG TESTS PASSED\n");

    // validate our memory manager
    validate();
  }
  
  // we should now be left with one free node
  print_all_nodes();
  printf("========================\n");
}


/*------------------------------------------------------*/


static void test_worst_fit()
{
  printf("WORSTFIT TEST\n");
  for(int i = 0; i < 5; i ++)
  {
    printf("[*] Running soak & merg tests on %d threads...\n",THREAD_NUMBER);
    initialise(memory_buffer, MEMORY_SIZE, WORSTFIT);
    start_test_threads();
    printf("[!] SOAK & MERG TESTS PASSED\n");

    // validate our memory manager
    validate();
  }
  
  // we should now be left with one free node
  print_all_nodes();
  printf("========================\n");

}


/*------------------------------------------------------*/


void main()
{
  test_first_fit();
  test_next_fit();
  test_best_fit();
  test_worst_fit();
}
