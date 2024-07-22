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
#define MAX_FUTEX_COUNT 1024*16
#define MAX_THREADS_PER_FUTEX 1024

struct wait_node {
	pthread_descr descr;
	char slot_is_used;
};

struct futex{
	struct wait_node wait_queue[MAX_THREADS_PER_FUTEX];
	unsigned long wait_queue_index;
	int spinlock;
	char slot_is_used;
	uint64_t *uaddress;
};

extern struct futex futex_list[MAX_FUTEX_COUNT];
extern unsigned long futex_last_used_index;
extern unsigned long futex_first_free_index;
extern int global_futex_sp;

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