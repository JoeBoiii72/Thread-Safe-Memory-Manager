/*
*----------------------------------------------------------------------------*
*  memory_manager.c                                                          *
*                                                                            *
*  Author: Joe Kenyon                                                        *
*                                                                            *
*  Last Updated: 06/12/2019                                                  *
*                                                                            *
*  Description: Basic thread-safe memory manager, allocate and deallocate    *
*               memory dynamically.                                          *
*                                                                            *
*               Uses either first-fit, next-fit, best-fit or worst-fit       *
*               algorithm to allocate memory. This is determined in the      *
*               initialize function.                                         *
*                                                                            *
*               Adjacent free memory blocks will be coalesced.               *
*----------------------------------------------------------------------------*
*/


#include "memory_manager.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <string.h>
#include <pthread.h>


// define the structure to hold each memory block
struct node_t
{
  struct node_t* next;
  struct node_t* prev;
  unsigned       free;
  size_t         size;
  uint8_t        memory[];
};

// linked list to store memory nodes
static struct node_t* linked_list;

//pointer to lastused/next node
static struct node_t* next_node;

//So it doesn't screw up when using threads.
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// sanity check allocations
static size_t heap_size;

void* (*allocate)(size_t bytes);
void* allocate_first_fit(size_t bytes);
void* allocate_next_fit(size_t bytes);
void* allocate_best_fit(size_t bytes);
void* allocate_worst_fit(size_t bytes);


/*...........................................................................*/
/*..                          DEBUGGING / VALIDATION                       ..*/
/*...........................................................................*/

// macros
#ifdef _DEBUG
  #define PRINTF                        printf
  #define PRINT_NODE(p)          print_node(p)
  #define PRINT_ALL_NODES()  print_all_nodes()
  #define VALIDATE()                validate()
  #define VALIDATE_NODE(p)    validate_node(p)
#else
  #define PRINTF
  #define PRINT_NODE(p)
  #define PRINT_ALL_NODES()
#endif

// if this fails, somethings gone wrong
// so use assert to tell the user.
void validate_node(struct node_t* p)
{
  assert(p->next == NULL || p->next->prev == p);
  assert(p->prev == NULL || p->prev->next == p);
  assert(p->size > 0);
}

void validate()
{
  pthread_mutex_lock(&lock);
  struct node_t* p = linked_list;
  size_t counter = 0;

  while (p)
  {
    validate_node(p);
    counter += p->size + sizeof(struct node_t);
    p = p->next;
  }

  // at any given point, nodes should sum to heap size.
  assert(counter == heap_size);
  pthread_mutex_unlock(&lock);
}

void print_node(struct node_t* p)
{
  printf("address[%10p] | " ,p);
  printf("size[%9zu] | "   ,p->size);
  printf("free[%1d]" ,p->free);
  if (next_node == p)
    printf(" <-");
  printf("\n");
}

void print_all_nodes()
{
  pthread_mutex_lock(&lock);
  struct node_t* p = linked_list;
  int i = 0;
  while(p)
  {
    printf("node[%5d] | ",i++);
    print_node(p);
    p = p->next;
  }
  pthread_mutex_unlock(&lock);
}

/*...........................................................................*/
/*..                          COMMON FUNCTIONS                             ..*/
/*...........................................................................*/


// dont leave empty blocks less than this or we could
// end up with a heap full of small blocks that cant/wont be allocated
#define MINIMUM_FREE_BLOCK 32
#define MINIMUM_HEAP_SIZE 1024

/**
*
* Creates a node at a given memory address
*
* @param memory : Address of memory where node is to be created.
* @param size : The size of the new node
*
* @return Pointer to a new node.
*
*/
static struct node_t* create_node(void* memory, size_t size)
{
  assert(memory);
  assert(size > sizeof(struct node_t));

  struct node_t* p = (struct node_t*)memory;

  // node info
  p->next = NULL;
  p->prev = NULL;
  p->free = 1;
  p->size = size - sizeof(struct node_t);
  return p;
}


/**
*
* Allocates memory for a given node
*
* @param p : Pointer to the node you want to allocate
* @param size : The amount of memory to be allocated
*
* @return Pointer to a new allocated node.
*
*/
static struct node_t* allocate_node(struct node_t* p, size_t bytes)
{
  assert(p);
  assert(bytes > 0);

  // calculate memory left over after allocation
  size_t remaining = p->size - bytes;

  if (remaining >= sizeof(struct node_t) + MINIMUM_FREE_BLOCK)
  {
    // create a new node with the remaining memory
    struct node_t* node = create_node(&p->memory[bytes], remaining);

    //update next and previous pointers for the new/current node
    node->next = p->next;
    node->prev = p;

    if (node->next)
      node->next->prev = node;

    p->next = node;
    p->size = bytes;
  }

  // mark as free and zero the memory
  p->free = 0;
  memset(p->memory, 0, p->size);

  return p;
}


/**
*
* Merges a given node with its previous node
*
* @param memory : Pointer to node to merge
*
* @return Pointer to the new merged node.
*
*/
static struct node_t* merge_prev(struct node_t* p)
{
  assert(p);

  // point to node after removed node
  p->prev->next = p->next;

  //adjust size of previous node
  p->prev->size += sizeof(struct node_t) + p->size;

  if (p->next)
    p->next->prev = p->prev;

  // current node should not exist, so return previous
  return p->prev;
}


/**
*
* Merges a given node with its next node
*
* @param memory : Pointer to node to merge
*
* @return Pointer to the new merged node.
*
*/
static struct node_t* merge_next(struct node_t* p)
{
  assert(p);

  // adjust size of current node
  p->size += sizeof(struct node_t) + p->next->size;
  
  p->next = p->next->next;

  if (p->next)
    p->next->prev = p;

  return p;
}

// initilizes memory manager
// full description in header file
void initialise(void* memory, size_t size, char* algorithm)
{
  // memory cannot be NULL
  // memor has to be of a minimum size
  assert(memory);
  assert(size > MINIMUM_HEAP_SIZE);

  // create a node containg all of free memory and point our list at it
  struct node_t* p = create_node(memory, size);

  heap_size   = size;

  // change head of linked list to point to this node
  linked_list = p;

  // we dont have a next/last-used node yet
  next_node   = NULL;

  // change allocate function pointer accordingly
  if (!algorithm || strcmp(algorithm, FIRSTFIT) == 0)
  {
    allocate = allocate_first_fit;
  }
  else if (strcmp(algorithm, NEXTFIT) == 0)
  {
    allocate = allocate_next_fit;
  }
  else if (strcmp(algorithm, BESTFIT) == 0)
  {
    allocate = allocate_best_fit;
  }
  else if (strcmp(algorithm, WORSTFIT) == 0)
  {
    allocate = allocate_worst_fit;
  } 
  else
  {
    fprintf(stderr, "Error : Unknown algorithm type\n");
    exit(EXIT_FAILURE);
  }
}

// Deallocates memory
// we need to also make sure we check if we are 
// destorying our last used node
// full description in header file
void deallocate(void* memory)
{  
  pthread_mutex_lock(&lock);

  // if deallocate was called before initialise
  assert(linked_list);

  // should be ok to pass in NULL
  if (memory == NULL)
  {
    pthread_mutex_unlock(&lock);
    return;
  }


  // check memory is is in heaps address space
  assert((uintptr_t)memory >= (uintptr_t)linked_list &&
    (uintptr_t)memory < (uintptr_t)linked_list + (uintptr_t)heap_size);

  // memory is a pointer to the data so recover the header
  struct node_t* p = ((struct node_t*)memory) - 1;


  // memory block should have been marked as in use
  // if it isnt we cant trust this block
  //assert(!p->free);
  if (p->free)
  {
	  fprintf(stderr, "Error : memory already free\n");
	  pthread_mutex_unlock(&lock);
	  return;
  }


  // make node free
  p->free = 1;

  // check prev block, increase size of prev if so
  if (p->prev && p->prev->free)
  {
    // Make sure we dont destroy our next/last used node
    if (next_node == p)
      next_node = p->next; // prev;

    p = merge_prev(p);
  }

  // check next block, and merg it if its free
  if (p->next && p->next->free)
  {
    // Make sure we dont destroy our next/last used node
    if (next_node == p->next)
      next_node = p->next->next;

    merge_next(p);
  }
  pthread_mutex_unlock(&lock);
}

/*...........................................................................*/
/*..                  ALLOCATION ALGORITHMS                                ..*/
/*...........................................................................*/


/**
 *
 * Returns a segment of dynamically allocated memory of the specified size.
 *
 * Uses the first free memory block that meets our size requirements.
 *
 * @param bytes : Bytes of memory to allocate
 *
 * @return Pointer to new block of memory
 *
*/
void* allocate_first_fit(size_t bytes)
{
  pthread_mutex_lock(&lock);
  assert(bytes > 0);

  // start at head of list
  struct node_t* p = linked_list;

  // allocate called before initialise
  assert(p);

  while (p)
  {
    // check its avavilable and we have room to allocate memory
    if (p->free && p->size >= bytes)
    {
      allocate_node(p, bytes);
      pthread_mutex_unlock(&lock);
      return p->memory;
    }

    // go to next node
    p = p->next;
  }
  pthread_mutex_unlock(&lock);
  return NULL;
}


/**
 *
 * Returns a segment of dynamically allocated memory of the specified size.
 *
 * Uses the first free memory block that meets our size
 * requirements. But starts at the last used memory segment.
 *
 * @param bytes : Bytes of memory to allocate
 *
 * @return Pointer to new block of memory
 *
*/
void* allocate_next_fit(size_t bytes)
{
  pthread_mutex_lock(&lock);
  assert(bytes > 0);

  // start at last used node
  struct node_t* p = next_node;

  // if null then start at head of linked list
  if (p == NULL)
    p = linked_list;

  // remember where we are
  struct node_t* posn = p;

  // this shouldn't be null
  assert(posn);

  // go through list until back to where we started
  do
  {
    // check its avavilable and we have room to allocate memory
    if (p->free && p->size >= bytes)
    {
      allocate_node(p, bytes);

      // update last used to next node as we know p is now not free
      next_node = p->next;
      pthread_mutex_unlock(&lock);
      return p->memory;
    }

    //reset to head of linked list
    p = p->next;
    if (p == NULL)
      p = linked_list;

  } while (p != posn);

  pthread_mutex_unlock(&lock);
  return NULL;
}


/**
 *
 * Returns a segment of dynamically allocated memory of the specified size.
 *
 * Uses the smallest possible memory block that fits our size requirements.
 *
 * @param bytes : Bytes of memory to allocate
 *
 * @return Pointer to new block of memory
 *
*/
void* allocate_best_fit(size_t bytes)
{
  pthread_mutex_lock(&lock);
  assert(bytes > 0);

  // start at head of list
  struct node_t* p = linked_list;

  // allocate called before initialise
  assert(p);

  size_t size = heap_size;

  // current smallest node
  struct node_t* smallest = NULL;

  // first go through all nodes and find the smallest valid node
  while (p)
  {
    if (p->free && p->size >= bytes && p->size < size)
    {
      smallest = p;
      size = p->size;
    }

    p = p->next;
  }

  // if we found a valid node
  if (smallest)
  {
    p = allocate_node(smallest, bytes);
    pthread_mutex_unlock(&lock);
    return p->memory;
  }

  // nothing found so return NULL
  pthread_mutex_unlock(&lock);
  return NULL;
}


/**
 *
 * Returns a segment of dynamically allocated memory of the specified size.
 *
 * Uses the largest possible memory block that fits our size requirements.
 *
 * @param bytes : Bytes of memory to allocate
 *
 * @return Pointer to new block of memory
 *
*/
void* allocate_worst_fit(size_t bytes)
{
  pthread_mutex_lock(&lock);

  assert(bytes > 0);

  // start at beginning of list
  struct node_t* p = linked_list;

  size_t size = bytes - 1;
  struct node_t* largest = NULL;

  // allocate called before initialise
  assert(p);

  // first go through all nodes and find the largest valid node
  while (p)
  {
    if (p->free && p->size > size)
    {
      largest = p;
      size = p->size;
    }

    p = p->next;
  }

  // if we found a valid node
  if (largest)
  {
    p = allocate_node(largest, bytes);
    pthread_mutex_unlock(&lock);
    return p->memory;
  }

  // nothing found so return NULL
  pthread_mutex_unlock(&lock);
  return NULL;
}