/*
*----------------------------------------------------------------------------*
*  memory_manager.h                                                          *
*                                                                            *
*  Author: Joe Kenyon                                                        *
*                                                                            *
*  Last Updated: 06/12/2019                                                  *
*                                                                            *
*  Description: Header file for Basic thread-safe memory manager, allocate   *
*               and deallocate memory dynamically.                           *
*                                                                            *
*               Uses either first-fit, next-fit, best-fit or worst-fit       *
*               algorithm to allocate memory. This is determined in the      *
*               initialize function.                                         *
*                                                                            *
*               Adjacent free memory blocks will be coalesced.               *
*----------------------------------------------------------------------------*
*/

#ifndef MEMORY_MANAGER_H__
#define MEMORY_MANAGER_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

  /**
   * Macros to aid initialise() call
  */
  #define FIRSTFIT "FirstFit"
  #define NEXTFIT  "NextFit"
  #define BESTFIT  "BestFit"
  #define WORSTFIT "WorstFit"

  /**
  *
  * Initializes the memory manager, creates the first memory node
  * and adds it to our linked list.
  *
  * @param memory : Pointer to a block of memory to use as the heap.
  * @param size : The size of the heap in bytes.
  * @param algorithm : Allocation algorithm to be used.
  *
  */
  void initialise(void* memory, size_t size, char* algorithm);


  /**
   *
   * Returns a segment of dynamically allocated memory of the specified size.
   *
   * Algorithm used depends on how the memory manager was initilized.
   *
   * @param bytes : Bytes of memory to allocate
   *
   * @return Pointer to new block of memory
   *
  */
  extern void* (*allocate)(size_t bytes);


  /**
   *
   * Frees a block of dynamically allocated memory
   * so that it can be recycled for later use.
   *
   *
   * @param memory : Pointer to a block of memory to deallocate.
   *                 Must be pointing somewhere in 
   *                 the heaps address space.
   *
  */
  void deallocate(void* memory);


  /**
   *
   * Prints all the nodes in our memory manager, along with their
   * attributes like size, address etc.
   *
  */
  void print_all_nodes();


  /**
   *
   * Validates the memory manager and checks if anything
   * has gone wrong.
   * 
   * We can check the sizes are correct by summing them and comparing
   * it with our heap size.
   * 
   * We can also check if nodes next and previous nodes are pointing to
   * where they should be e.g. p->next->prev must equal p.
   * 
   * If something has gone wrong our asserts 
   * will stop the program from running
   * 
  */
  void validate();
  
#ifdef __cplusplus
}
#endif

#endif