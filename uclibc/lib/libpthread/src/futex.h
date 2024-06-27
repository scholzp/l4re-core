// #include "pthread.h"
// #include "internals.h"
// #include "spinlock.h"
// #include "restart.h"
#include <stdlib.h>
#include "assert.h"
#include "pthread.h"
#include "internals.h"

// Prototypes
// int futex_wait(uint64_t *uaddr, unsigned int flags, u32 val, ktime_t *abs_time, u32 bitset);
// int __futex_wait(uint64_t *uaddr, unsigned int flags, u32 val, struct hrtimer_sleeper *to, u32 bitset)
// int futex_wait_setup(uint64_t *uaddr, u32 val, unsigned int flags,
// 			 struct futex_q *q, struct futex_hash_bucket **hb);
// int futex_wake(uint64_t *uaddr, unsigned int flags, int nr_wake, u32 bitset);

// Structs an so on


// Wait node of a thread
struct wait_node {
	struct wait_node *next;	/* Next node in null terminated linked list */
	struct wait_node *prev;	/* Next node in null terminated linked list */
	pthread_descr thr;		/* The thread waiting with this node */
};

struct futex{
	struct wait_node *sleep_queue_head; // Wait nodes of threads sleeping on this futex
	struct wait_node *sleep_queue_tail;
	int spinlock;
	int queue_dirty;
};

struct futex_node{
	struct futex *futex;	 // Pointer to the futex of this entry
	struct futex_node *next; // Pointer to the next entry
	struct futex_node *prev; // Pointer to the next entry
	uint64_t *uaddress;	  // User address this futex belongs to.
	int spinlock;
};

// Simply create a futex and return the pointer
void create_futex(struct futex **futex);
// Respective function to clean up memory
void destroy_futex(struct futex *futex);

void add_wait_node(struct futex *futex, pthread_descr descr);

int remove_wait_node(struct futex *futex, pthread_descr descr);

struct wait_node **find_wait_node(struct futex *futex, pthread_descr descr);

int find_futex(struct futex_node *head, struct futex **futex,
				uint64_t *address);

void add_futex_node(struct futex_node **head, struct futex **futex,
					uint64_t *address);

// Respective function to clean up memory
void destroy_futex_list(struct futex_node *head);

// global futex list head
extern struct futex_node *global_futex_list;

/*
 * Put the current thread on the sleep queue of the futex at address
 * ``uaddr''.  Let it sleep for the specified ``timeout'' time, or
 * indefinitely if the argument is NULL.
 */
int
futex_wait(uint64_t *uaddr, uint64_t val, const struct timespec *timeout,
		int flags);

/*
 * Wakeup at most ``n'' sibling threads sleeping on a futex at address
 * ``uaddr'' and requeue at most ``m'' sibling threads on a futex at
 * address ``uaddr2''.
 */
unsigned long 
futex_requeue(uint64_t *uaddr, uint64_t n, uint64_t *uaddr2, uint64_t m,
	int flags);

/*
 * Wakeup at most ``n'' sibling threads sleeping on a futex at address
 * ``uaddr''.
 */
int
futex_wake(uint64_t *uaddr, uint64_t n, int flags);