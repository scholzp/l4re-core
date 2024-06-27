/* Linuxthreads - a simple clone()-based implementation of Posix        */
/* threads for Linux.                                                   */
/* Copyright (C) 1998 Xavier Leroy (Xavier.Leroy@inria.fr)              */
/*                                                                      */
/* This program is free software; you can redistribute it and/or        */
/* modify it under the terms of the GNU Library General Public License  */
/* as published by the Free Software Foundation; either version 2       */
/* of the License, or (at your option) any later version.               */
/*                                                                      */
/* This program is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU Library General Public License for more details.                 */

/* Internal locks */

#include <errno.h>
//l4/#include <sched.h>
#include <time.h>
#include <stdlib.h>
#include <limits.h>
#include "pthread.h"
#include "internals.h"
#include "spinlock.h"
#include "restart.h"
#include "futex.h"

#include <l4/sys/thread.h>
#include <l4/util/util.h>

void __pthread_release(int * spinlock)
{
  WRITE_MEMORY_BARRIER();
  *spinlock = __LT_SPINLOCK_INIT;
  __asm__ __volatile__ ("" : "=m" (*spinlock) : "m" (*spinlock));
}


/* The status field of a spinlock is a pointer whose least significant
   bit is a locked flag.

   Thus the field values have the following meanings:

   status == 0:       spinlock is free
   status == 1:       spinlock is taken; no thread is waiting on it

   (status & 1) == 1: spinlock is taken and (status & ~1L) is a
                      pointer to the first waiting thread; other
		      waiting threads are linked via the p_nextlock
		      field.
   (status & 1) == 0: same as above, but spinlock is not taken.

   The waiting list is not sorted by priority order.
   Actually, we always insert at top of list (sole insertion mode
   that can be performed without locking).
   For __pthread_unlock, we perform a linear search in the list
   to find the highest-priority, oldest waiting thread.
   This is safe because there are no concurrent __pthread_unlock
   operations -- only the thread that locked the mutex can unlock it. */


void internal_function __pthread_lock(struct _pthread_fastlock * lock,
				      pthread_descr self)
{
  {
    __pthread_acquire(&lock->__spinlock);
    return;
  }
}

int __pthread_unlock(struct _pthread_fastlock * lock)
{
  {
    __pthread_release(&lock->__spinlock);
    return 0;
  }
}

/*
 * Alternate fastlocks do not queue threads directly. Instead, they queue
 * these wait queue node structures. When a timed wait wakes up due to
 * a timeout, it can leave its wait node in the queue (because there
 * is no safe way to remove from the quue). Some other thread will
 * deallocate the abandoned node.
 */

inline unsigned long long time_diff_to_ns(struct timespec* start, struct timespec* end) {
  if (end->tv_nsec > start->tv_nsec)
    return (end->tv_sec - start->tv_sec) * 1000000000 + (end->tv_nsec - start->tv_nsec);
  else 
    return (end->tv_sec - start->tv_sec - 1) * 1000000000 + (1000000000 + end->tv_nsec - start->tv_nsec);
}

struct wait_node_pt {
  struct wait_node_pt *next;	/* Next node in null terminated linked list */
  pthread_descr thr;		/* The thread waiting with this node */
  int abandoned;		/* Atomic flag */
};

static long wait_node_pt_free_list;
static int wait_node_pt_free_list_spinlock;

/* Allocate a new node from the head of the free list using an atomic
   operation, or else using malloc if that list is empty.  A fundamental
   assumption here is that we can safely access wait_node_pt_free_list->next.
   That's because we never free nodes once we allocate them, so a pointer to a
   node remains valid indefinitely. */

static struct wait_node_pt *wait_node_pt_alloc(void)
{
    struct wait_node_pt *new_node = 0;

    __pthread_acquire(&wait_node_pt_free_list_spinlock);
    if (wait_node_pt_free_list != 0) {
      new_node = (struct wait_node_pt *) wait_node_pt_free_list;
      wait_node_pt_free_list = (long) new_node->next;
    }
    WRITE_MEMORY_BARRIER();
    __pthread_release(&wait_node_pt_free_list_spinlock);

    if (new_node == 0)
      return malloc(sizeof *wait_node_pt_alloc());

    return new_node;
}

/* Return a node to the head of the free list using an atomic
   operation. */

static void wait_node_pt_free(struct wait_node_pt *wn)
{
    __pthread_acquire(&wait_node_pt_free_list_spinlock);
    wn->next = (struct wait_node_pt *) wait_node_pt_free_list;
    wait_node_pt_free_list = (long) wn;
    WRITE_MEMORY_BARRIER();
    __pthread_release(&wait_node_pt_free_list_spinlock);
    return;
}

void __pthread_alt_lock(struct _pthread_fastlock * lock,
		        pthread_descr self)
{
  // struct wait_node_pt wait_node_pt;
  // {
  //   int suspend_needed = 0;
  //   __pthread_acquire(&lock->__spinlock);

  //   if (lock->__status == 0)
  //     lock->__status = 1;
  //   else {
  //     if (self == NULL)
	//     self = thread_self();

  //     wait_node_pt.abandoned = 0;
  //     wait_node_pt.next = (struct wait_node_pt *) lock->__status;
  //     wait_node_pt.thr = self;
  //     lock->__status = (long) &wait_node_pt;
  //     suspend_needed = 1;
  //   }

  //   __pthread_release(&lock->__spinlock);
  //   if (suspend_needed){
  //     suspend (self);
  //   }
  //   // __pthread_lock(lock, self);
  //   return;
  // }
  // int val = __atomic_sub_fetch(&lock->__status, 1, __ATOMIC_SEQ_CST);
  // // Fast path
  //     // printf("Value %d\n", val);
  // if (-1 == val) {
  //   return;
  // } else {
  //   // Spin until we either sleep or access the fast path
  //   while (1 != futex_wait(&lock->__status, val, NULL, 0)) {
  //     val = __atomic_sub_fetch(&lock->__status, 1, __ATOMIC_SEQ_CST);
  //     // if we can access the fast path, simply return
  //     if (-1 == val)
  //       return; 
  //   }
  // }
            uint64_t c; 
        	if ((c = __sync_val_compare_and_swap(&lock->__status, 0, 1)) != 0) {
        		do {
        			if (c == 2 || __sync_val_compare_and_swap(&lock->__status, 1, 2) != 0) {
        				futex_wait(&lock->__status, 2, NULL, 0);
        			}
        		} while ((c = __sync_val_compare_and_swap(&lock->__status, 0, 2)) != 0);
        	}
}

/* Timed-out lock operation; returns 0 to indicate timeout. */

int __pthread_alt_timedlock(struct _pthread_fastlock * lock,
			    pthread_descr self, const struct timespec *abstime)
{
//   long oldstatus = 0;
// #if defined HAS_COMPARE_AND_SWAP
//   long newstatus;
// #endif
//   struct wait_node_pt *p_wait_node_pt = wait_node_pt_alloc();

//   /* Out of memory, just give up and do ordinary lock. */
//   if (p_wait_node_pt == 0) {
//     __pthread_alt_lock(lock, self);
//     return 1;
//   }

// #if defined TEST_FOR_COMPARE_AND_SWAP
//   if (!__pthread_has_cas)
// #endif
// #if !defined HAS_COMPARE_AND_SWAP || defined TEST_FOR_COMPARE_AND_SWAP
//   {
//     __pthread_acquire(&lock->__spinlock);

//     if (lock->__status == 0)
//       lock->__status = 1;
//     else {
//       if (self == NULL)
// 	self = thread_self();

//       p_wait_node_pt->abandoned = 0;
//       p_wait_node_pt->next = (struct wait_node_pt *) lock->__status;
//       p_wait_node_pt->thr = self;
//       lock->__status = (long) p_wait_node_pt;
//       oldstatus = 1; /* force suspend */
//     }

//     __pthread_release(&lock->__spinlock);
//     goto suspend;
//   }
// #endif

// #if defined HAS_COMPARE_AND_SWAP
//   do {
//     oldstatus = lock->__status;
//     if (oldstatus == 0) {
//       newstatus = 1;
//     } else {
//       if (self == NULL)
// 	self = thread_self();
//       p_wait_node_pt->thr = self;
//       newstatus = (long) p_wait_node_pt;
//     }
//     p_wait_node_pt->abandoned = 0;
//     p_wait_node_pt->next = (struct wait_node_pt *) oldstatus;
//     /* Make sure the store in wait_node_pt.next completes before performing
//        the compare-and-swap */
//     MEMORY_BARRIER();
//   } while(! __compare_and_swap(&lock->__status, oldstatus, newstatus));
// #endif

// #if !defined HAS_COMPARE_AND_SWAP || defined TEST_FOR_COMPARE_AND_SWAP
//   suspend:
// #endif

//   /* If we did not get the lock, do a timed suspend. If we wake up due
//      to a timeout, then there is a race; the old lock owner may try
//      to remove us from the queue. This race is resolved by us and the owner
//      doing an atomic testandset() to change the state of the wait node from 0
//      to 1. If we succeed, then it's a timeout and we abandon the node in the
//      queue. If we fail, it means the owner gave us the lock. */

//   if (oldstatus != 0) {
//     if (timedsuspend(self, abstime) == 0) {
//       if (!testandset(&p_wait_node_pt->abandoned))
// 	return 0; /* Timeout! */

//       /* Eat oustanding resume from owner, otherwise wait_node_pt_free() below
// 	 will race with owner's wait_node_pt_dequeue(). */
//       suspend(self);
//     }
//   }

//   wait_node_pt_free(p_wait_node_pt);

//   READ_MEMORY_BARRIER();

  return 1; /* Got the lock! */
}

void __pthread_alt_unlock(struct _pthread_fastlock *lock)
{
  // __pthread_unlock(lock);
  // struct wait_node_pt *p_node, **pp_node, *p_max_prio, **pp_max_prio;
  // struct wait_node_pt ** const pp_head = (struct wait_node_pt **) &lock->__status;
  // int maxprio;

  // unsigned long long retires = 0;
  // unsigned long long cycles = 0; 

  // WRITE_MEMORY_BARRIER();

  // __pthread_acquire(&lock->__spinlock);
  // while (1) {
  // /* If no threads are waiting for this lock, try to just
  //    atomically release it. */
  //   {
  //     if (lock->__status == 0 || lock->__status == 1) {
	//       lock->__status = 0;
	//       break;
  //     }
  //   }
  //   /* Process the entire queue of wait nodes. Remove all abandoned
  //      wait nodes and put them into the global free queue, and
  //      remember the one unabandoned node which refers to the thread
  //      having the highest priority. */

  //   pp_max_prio = pp_node = pp_head;
  //   p_max_prio = p_node = *pp_head;
  //   maxprio = INT_MIN;

  //   READ_MEMORY_BARRIER(); /* Prevent access to stale data through p_node */
  //   while (p_node != (struct wait_node_pt *) 1) {
  //     ++cycles;
  //     int prio;
  //     if (p_node->abandoned) {
  //   	  *pp_node = p_node->next;
  //     	// wait_node_pt_free(p_node);
  //       /* Note that the next assignment may take us to the beginning
  //       of the queue, to newly inserted nodes, if pp_node == pp_head.
  //       In that case we need a memory barrier to stabilize the first of
  //       these new nodes. */
  //       p_node = *pp_node;
	//       if (pp_node == pp_head)
	//         // READ_MEMORY_BARRIER(); /* No stale reads through p_node */
	//       continue;
  //     } else if (p_node->thr->p_priority >= maxprio) {
  //         prio = p_node->thr->p_priority;
  //         maxprio = prio;
  //         pp_max_prio = pp_node;
  //         p_max_prio = p_node;
  //       /* Otherwise remember it if its thread has a higher or equal priority
  //         compared to that of any node seen thus far. */
  //     }
  //     /* This canno6 jump backward in the list, so no further read
  //        barrier is needed. */
  //     pp_node = &p_node->next;
  //     p_node = *pp_node;
  //   }

  //   /* If all threads abandoned, go back to top */
  //   if (maxprio == INT_MIN)
  //     continue;

  //   /* Now we want to to remove the max priority thread's wait node from
  //      the list. Before we can do this, we must atomically try to change the
  //      node's abandon state from zero to nonzero. If we succeed, that means we
  //      have the node that we will wake up. If we failed, then it means the
  //      thread timed out and abandoned the node in which case we repeat the
  //      whole unlock operation. */

  //   if (!testandset(&p_max_prio->abandoned)/*(p_max_prio->abandoned)*/) {
  //   	*pp_max_prio = p_max_prio->next;
  //     /* Release the spinlock *before* restarting.  */
	//     __pthread_release(&lock->__spinlock);
  //     restart(p_max_prio->thr);
  //     // l4_usleep(1);
  //     return;
  //   }
  //   ++retires;
  // }
  // __pthread_release(&lock->__spinlock);
             	if (__sync_sub_and_fetch(&lock->__status, 1) != 0) {
                // We *really* don't want to miss any wait queue entries...
		        lock->__status = 0;
                __sync_synchronize();
		        futex_wake(&lock->__status, 1, 0);
        	}
}


/* Compare-and-swap emulation with a spinlock */

#ifdef TEST_FOR_COMPARE_AND_SWAP
int __pthread_has_cas = 0;
#endif

#if !defined HAS_COMPARE_AND_SWAP || defined TEST_FOR_COMPARE_AND_SWAP

int __pthread_compare_and_swap(long * ptr, long oldval, long newval,
                               int * spinlock)
{
  int res;

  __pthread_acquire(spinlock);

  if (*ptr == oldval) {
    *ptr = newval; res = 1;
  } else {
    res = 0;
  }

  __pthread_release(spinlock);

  return res;
}

#endif

/* The retry strategy is as follows:
   - We test and set the spinlock MAX_SPIN_COUNT times, calling
     sched_yield() each time.  This gives ample opportunity for other
     threads with priority >= our priority to make progress and
     release the spinlock.
   - If a thread with priority < our priority owns the spinlock,
     calling sched_yield() repeatedly is useless, since we're preventing
     the owning thread from making progress and releasing the spinlock.
     So, after MAX_SPIN_LOCK attemps, we suspend the calling thread
     using nanosleep().  This again should give time to the owning thread
     for releasing the spinlock.
     Notice that the nanosleep() interval must not be too small,
     since the kernel does busy-waiting for short intervals in a realtime
     process (!).  The smallest duration that guarantees thread
     suspension is currently 2ms.
   - When nanosleep() returns, we try again, doing MAX_SPIN_COUNT
     sched_yield(), then sleeping again if needed. */

void __pthread_acquire(int * spinlock)
{
  int cnt = 0;

  READ_MEMORY_BARRIER();

  while (testandset(spinlock)) {
    if (cnt < MAX_SPIN_COUNT) {
      l4_thread_yield();
      cnt++;
    } else {
      l4_usleep(SPIN_SLEEP_DURATION / 1000);
      cnt = 0;
    }
  }
// while (!__sync_bool_compare_and_swap(spinlock, 0, 1)) while (*spinlock){
//     __builtin_ia32_pause();
// }
//   while (1) {
//     if ((*spinlock) == __LT_SPINLOCK_INIT) {
//       if (!testandset(spinlock))
//         break;
//     }
//     if (cnt < MAX_SPIN_COUNT) {
//       l4_thread_yield();
//       cnt++;
//     } else {
//       l4_usleep(SPIN_SLEEP_DURATION / 1000);
//       cnt = 0;
//     }
//   }
}
