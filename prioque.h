//
// Priority Queue (c) 1985-2022 by Golden G. Richard III,
// Ph.D. (@nolaforensix)
//
// Major update October 2004: The package is now thread-safe.  The
// package now depends on the pthreads library (for access to
// mutexes). All functions now take one or more pointers to a queue
// or context, so existing applications will need to be modified
// slightly.
//
// April 2007: added an option to init_queue to allow priority field
// to serve only as a tag--the queue is not sorted by the priority
// field when 'priority_is_tag_only' is set and the queue operates as
// a strict FIFO data structure.
//
// September 2017: Minor code cleanup and fix to add_to_queue to
// ensure strict add-to-rear for queues with equal priority elements.
//
// March 2021:

// o Minor cleanup + minor changes to API that may break older code
// and require minor changes to compile.
//
// o remove_from_front(), peek_at_current(), and
// pointer_to_current() now return a non-NULL value if the queue isn't
// empty or return NULL if the queue is empty (rather than failing
// with a consistency check). This behavior is more multithreading
// friendly and in particular, for remove_from_front(), provides an
// atomic method for checking for an element in the queue *and*
// removing the element.
//
// o add_to_queue() was rewritten to simplify the implementation.  See
// the documentation for add_to_queue below to verify that the new
// behavior is what you expect.
//
// o Based on valgrind analysis, some potential thread safety issues
// were addressed.
//
// o queue lengths are now unsigned longs instead of unsigned ints.
//
// August 2021: optimized insertion speed for strict FIFO
// queues. Deleted local view functions, as the semantics are too
// complicated in the face of global queue updates and they are no
// longer used in any production code that I'm aware of.  Rewrote
// copy_queue().
//

#if ! defined(QUEUE_TYPE_DEFINED)
#define QUEUE_TYPE_DEFINED

#include <pthread.h>

#define  TRUE  1
#define  FALSE 0
#define CONSISTENCY_CHECKING

// type of one element in a queue 

typedef struct _Queue_element {
  void *info;
  int priority;
  struct _Queue_element *next;
} *Queue_element;

// basic queue type 

typedef struct Queue {
  Queue_element queue;		                     // head of queue
  Queue_element tail;		                     // tail of queue
  Queue_element current;	                     // current position for sequential
                                                     //access functions 
  Queue_element previous;	                     // one step back from current 
  unsigned long queuelength;	                     // # of elements in queue 
  unsigned int elementsize;	                     // 'sizeof()' one element 
  unsigned int duplicates;	                     // are duplicates allowed? 
  int (*compare) (const void *e1, const void *e2);   // element comparision function 
  pthread_mutex_t lock;
  int priority_is_tag_only;                          // if TRUE, ignore priority and use
                                                     // strict FIFO
} Queue;


//********
//
// NOTE: init_queue() must be called for a new queue before any other "prioque.c" 
// functions are called.
//
/********/


// function prototypes and descriptions for visible "prioque.c" functions

////////////////////////////
// SECTION 1
////////////////////////////

/* initializes a new queue 'q' to have elements of size 'elementsize'.
   If 'duplicates' is true, then duplicate elements in the queue are
   allowed, otherwise duplicates are silently deleted.  The
   element-comparing function 'compare' is required only if
   duplicates==FALSE or either equal_queues() or element_in_queue()
   are used (otherwise, a null function is acceptable).  'compare'
   should be a standard qsort()-style element comparison function:
   returns 0 if elements match, otherwise a non-0 result (<, > cases
   are not used).

   IMPORTANT: *Only* the 'compare' function is used for duplicate
   detection.
*/
void init_queue(Queue *q, unsigned int elementsize, unsigned int duplicates,
		 int (*compare) (const void *e1, const void *e2),
		 unsigned int priority_is_tag_only);


/* destroys all elements in 'q'
*/
void destroy_queue(Queue *q);


/* adds 'element' to the 'q' with position based on 'priority'.
   Elements with higher-numbered priorities are placed closer to the
   front of the queue, with strict 'to the rear' placement for items
   with equal priority (that is, given two items with equal priority,
   the one most recently added will appear closer to the rear of the
   queue).
*/
void add_to_queue(Queue *q, void *element, int priority);


/* removes the element at the front of the 'q' and places it in 'element'. 
   If the queue is empty, returns NULL, otherwise a non-NULL value.
*/
void *remove_from_front(Queue *q, void *element);


/* returns TRUE if the 'element' exists in the 'q', otherwise false.
   The 'compare' function is used for matching.  As a side-effect, the
   current position in the queue is set to matching element, so
   'update_current()' can be used to update the value of the
   'element'.  If the element is not found, the current position is
   set to the first element in the queue.
*/
unsigned int element_in_queue(Queue *q, void *element);


/* returns TRUE if 'q' is empty, FALSE otherwise 
*/
unsigned int empty_queue(Queue *q);


/* returns the number of elements in the 'q'.
*/
unsigned long queue_length(Queue *q);


/* makes a copy of 'q2' into 'q1'.  'q2' is not modified.
*/
void copy_queue(Queue *q1, Queue *q2);


/* determines if 'q1' and 'q2' are equivalent.  Uses the 'compare'
   function of the first queue, which should match the 'compare' for
   the second!  Returns TRUE if the queues are equal, otherwise
   returns FALSE.
*/
unsigned int equal_queues(Queue *q1, Queue *q2);


/* merge 'q2' into 'q1'.   'q2' is not modified.
*/
void merge_queues(Queue *q1, Queue *q2);


////////////////////////////
// SECTION 2
////////////////////////////

/* the following are functions used to "walk" the queue (globally)
   like a linked list, examining or deleting elements along the way.
   Current position is rewound to the beginning by functions above (in
   SECTION 1), except for 'empty_queue()' and 'queue_length()', which
   do not modify the global current position.
*/

/********************/
/********************/

/* move to the first element in the 'q' */
void rewind_queue (Queue *q);


/* move to the next element in the 'q' */
void next_element (Queue *q);


/* allows update of current element.  The priority should not
   be changed by this function!
*/

void update_current (Queue *q, void *element);


/* peek at the element stored at the current position in the 'q'.
   Returns NULL if the queue is empty or a non-NULL value otherwise */
void *peek_at_current (Queue *q, void *element);


/* return a pointer to the current element.  Returns NULL if the 
   queue is empty or there is no current element. */
void *pointer_to_current (Queue *q);


/* return priority of current element in the 'q' */
int current_priority (Queue *q);


/* delete the element stored at the current position */
void delete_current (Queue *q);


/* has the current position in 'q'  moved beyond the last valid element?  
   Returns TRUE if so, FALSE otherwise.
*/
unsigned int end_of_queue (Queue *q);

#endif


/*** QUICK REFERENCE ***

// SECTION 1
void init_queue(Queue *q, int elementsize, int duplicates, 
		int (*compare)(void *e1, void *e2), int priority_is_tag_only);
void destroy_queue(Queue *q);
void add_to_queue(Queue *q, void *element, int priority);
void remove_from_front(Queue *q, void *element);
unsigned int element_in_queue(Queue *q, void *element);
unsigned int empty_queue(Queue *q);
unsigned int queue_length(Queue *q);
void copy_queue(Queue *q1, Queue *q2);
unsigned int equal_queues(Queue q1, Queue *q2);
void merge_queues(Queue *q1, Queue *q2);

// SECTION 2

void rewind_queue(Queue *q);
void next_element(Queue *q);
void update_current(Queue *q, void *element);
void peek_at_current(Queue *q, void *element);
void *pointer_to_current(Queue *q);
int current_priority(Queue *q);
void delete_current(Queue *q);
unsigned int end_of_queue(Queue *q);

 *** QUICK REFERENCE ***/
