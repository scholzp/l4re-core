#include "futex.h"
#include "internals.h"
#include "spinlock.h"
#include "restart.h"
#include "errno.h"

#include <l4/sys/thread.h>
#include <l4/util/util.h>
#include <l4/sys/kdebug.h>


#define DEBUG_MSG(msg)              \
    // outstring(msg); \
	//("TSC %llu: Thread %lu  Line: %lu: %s\n",__builtin_ia32_rdtsc(), thread_self(), __LINE__, msg); \
    // __pthread_release(&out_lock); \

struct futex_node *global_futex_list;
static int global_futex_sp = 0;

void inline __sl_acquire(int *sl) {
    while (!__sync_bool_compare_and_swap((sl), 0, 1)) while (*sl) __builtin_ia32_pause();
}

void inline  __sl_release(int * spinlock)
{
  __sync_synchronize();
  *spinlock = 0;
  __asm__ __volatile__ ("" : "=m" (*spinlock) : "m" (*spinlock));
}

// Simply create a futex and return the pointer
void create_futex(struct futex **futex) {
	__sync_synchronize();
	DEBUG_MSG("Create futex")
	(*futex) = (struct futex*) malloc(sizeof(struct futex));
	(*futex)->sleep_queue_head = NULL;
	(*futex)->sleep_queue_tail = NULL;
	(*futex)->queue_dirty = 0;
	(*futex)->spinlock = 0;
	__sync_synchronize();
}

// Respective function to clean up memory
void destroy_futex(struct futex *futex) {
	__sl_acquire(&global_futex_sp);
	__sync_synchronize();
	DEBUG_MSG("destroy futex");
	if (futex == NULL) {
		__sl_release(&global_futex_sp);	
		return;
	}
	if (futex->sleep_queue_head) {
		// Remove all nodes
		struct wait_node *current = futex->sleep_queue_head;
		while (current) {
			// printf("Delete wait_node %p\n", current);
			struct wait_node *to_free = current;
			current = current->next;
			free(to_free); 
		}
	}
	free(futex);
	__sync_synchronize();
	__sl_release(&global_futex_sp);
}

void add_wait_node(struct futex *futex, pthread_descr descr){
	__sync_synchronize();
	DEBUG_MSG("add_wait_node");
	if(NULL == futex) {
		DEBUG_MSG("FUTEX NULL; DONT INSERT NODE");
		return;
	}
	if(NULL == futex->sleep_queue_head) {
		__sync_synchronize();
		DEBUG_MSG("CREATE NEW HEAD");
		futex->sleep_queue_head = (struct wait_node *) malloc(sizeof(*futex->sleep_queue_head));
		futex->sleep_queue_head->next = NULL;
		futex->sleep_queue_head->prev = NULL;
		futex->sleep_queue_head->thr = descr;
		futex->sleep_queue_tail = futex->sleep_queue_head;
		__sync_synchronize();
	} else {
		if (NULL != (find_wait_node(futex, descr))){
			return;
			// printf("Add note a second time!\n");
		}
		DEBUG_MSG("Queue length >=2");
		__sync_synchronize();
		struct wait_node *new_tail = (struct wait_node*) malloc(sizeof(*new_tail));
		// printf("Allocated at address %p\n", new_tail);
		new_tail->thr = descr;
		new_tail->next = NULL;
		new_tail->prev = futex->sleep_queue_tail;
		futex->sleep_queue_tail->next = new_tail;
		futex->sleep_queue_tail = new_tail;
		__sync_synchronize();
	}
}

struct wait_node *pop_wait_node(struct futex *futex){
	DEBUG_MSG("Pop head!");
	if (NULL != futex->sleep_queue_head) {
		struct wait_node *old_head = futex->sleep_queue_head;
		futex->sleep_queue_head = futex->sleep_queue_head->next;
		if (NULL != futex->sleep_queue_head) {
			DEBUG_MSG("Set prev of head to NULL");
			futex->sleep_queue_head->prev = NULL;
		}
		DEBUG_MSG("Return result");
		return old_head;
	} else {
		DEBUG_MSG("Retrun NULL");
		return NULL;
	}
}

int remove_wait_node(struct futex *futex, pthread_descr descr){
	__sync_synchronize();
	DEBUG_MSG("remove_wait_node");

	struct wait_node **to_remove = find_wait_node(futex,  descr);
	if ((NULL != (to_remove)) && (NULL != (*to_remove))) {
		struct wait_node *to_free = *to_remove;
		if (NULL != (*to_remove)->prev) {
			(*to_remove)->prev->next = to_free->next;
		}
		(*to_remove) = to_free->next;
		free(to_free);
		return 1;
	} else {
		DEBUG_MSG("Nothing removed");
		return -1;
	}
}

struct wait_node **find_wait_node(struct futex *futex, pthread_descr descr) {
	__sync_synchronize();
	int pos = 0;
	DEBUG_MSG("find_wait_node");
	if (NULL == futex) {
		DEBUG_MSG("wait_node list not initialized!");
		return NULL;
	}
	if (NULL == futex->sleep_queue_head) {
		DEBUG_MSG("find_wait_node: Sleep queue head NULL");
		return NULL;
	}
	DEBUG_MSG("Sleep queue head Not null; Enter while...");
	struct wait_node **current = &futex->sleep_queue_head;
	while (((*current) != NULL)) {
		DEBUG_MSG("Iterate through queue.");
		if ((*current)->thr == descr) {
			// printf("WAIT_Node at pos %d\n", pos);
			return current;
		}
		current = &((*current)->next);
		++pos;
	}
	DEBUG_MSG("Return NULL");
	return NULL;
}

/*
* Finds a futex for the given address, if it exists.
* If it exists, return 1 and the pointer to the futex.
* If not, return 0 and do nothing with the provided pointer.
*/ 
int find_futex(struct futex_node *head, struct futex **futex, uint64_t *address){
	__sync_synchronize();
	int result = 0;
	DEBUG_MSG("find_futex");

	if (NULL != head)
	{
		if (head->uaddress == address) {
			DEBUG_MSG("head");
			*futex = head->futex;
			result = 1;
		} else {
			struct futex_node *current = head->next; 
			while (current != head) {
				if (current->uaddress == address) {
					*futex = current->futex;
					result = 1;
					break;
				}
				current = current->next;
			}
		}
	}
	else {
		DEBUG_MSG("FUTEX not found, HEAD NULL")
	}
	DEBUG_MSG("Return futex;");
	return result;
}

void add_futex_node(struct futex_node **head, struct futex **futex, uint64_t *address){
	// Does this futex exist?
	DEBUG_MSG("add_futex_node");
	__sync_synchronize();
	if (find_futex(*head, futex, address))
		return;
	else {
		DEBUG_MSG("Make new futex");
		create_futex(futex);
	}
	// Futex not in the list...
	if (NULL == (*head)) {
		DEBUG_MSG("HEad NULL");
		// No list existing, create one
		(*head) = (struct futex_node*) malloc(sizeof(**head));
		(*head)->futex = *futex;
		(*head)->next = (*head);
		(*head)->prev = (*head);
		(*head)->uaddress = address;
		(*head)->spinlock = 0;
		DEBUG_MSG("Created new Futex an initialized");
		__sync_synchronize();
	} else {
		DEBUG_MSG("HEAD not NULL");
		// List existing, Futex need to be added
		struct futex_node *new_tail = (struct futex_node*) malloc(sizeof(*new_tail));
		struct futex_node *current_tail = (*head)->prev; 
		// Attach the new tail the as current tail's successor
		new_tail->prev = current_tail;
		new_tail->next = (*head);
		// Attach the new tail to the head as it's successor
		(*head)->prev = new_tail;
		current_tail->next = new_tail;
		new_tail->futex = *futex;
		new_tail->uaddress = address;
		__sync_synchronize();
	}
	//WRITE_MEMORY_BARRIER();
}

// Respective function to clean up memory
void destroy_futex_list(struct futex_node *head) {
	__sync_synchronize();
	__sl_acquire(&global_futex_sp);
	if (NULL != head) {
		// Detach the tail node from the head
		head->prev->next = NULL;
		// Remove all nodes
		struct futex_node *current = head;
		while (current) {
			struct futex_node *to_free = current;
			destroy_futex(to_free->futex);
			current = current->next;
			free(to_free); 
		}
	}
	__sl_release(&global_futex_sp);
}

/*
 * Put the current thread on the sleep queue of the futex at address
 * ``uaddr''.  Let it sleep for the specified ``timeout'' time, or
 * indefinitely if the argument is NULL.
 */
int
futex_wait(uint64_t *uaddr, uint64_t val, const struct timespec *timeout,
	int flags)
{
	__sl_acquire(&global_futex_sp);
	__sync_synchronize();
	DEBUG_MSG("futex_wait()");
	// struct proc *p = curproc;
	struct futex *f;
	// uint64_t nsecs = INFSLP;
	int error = -1;
	DEBUG_MSG("Gonna wait");
	/*
	 * After reading the value a race is still possible but
	 * we deal with it by serializing all futex syscalls.
	 */
	/* If the value changed, stop here. */
	uint64_t val2 = __atomic_load_n(uaddr, __ATOMIC_SEQ_CST); 
	if ((val2 != val)) {
		DEBUG_MSG("Value changed!");
		__sl_release(&global_futex_sp);
		return EAGAIN;
	}

	pthread_descr t_descr = thread_self();

	// __sl_acquire(&global_futex_sp);
	__sync_synchronize();
	add_futex_node(&global_futex_list, &f, uaddr);
	if(f == NULL) {
		DEBUG_MSG("FUTEX NULL; DIDNT EXPECT");
		return EAGAIN;
	} else {
		DEBUG_MSG("FUTEX NOT NULL");
	}
	
	if ((find_wait_node(f, t_descr)) != NULL) {
		DEBUG_MSG("Already queued!");
	}

	// if (timeout != NULL) {
	// TODO: Timeout stuff
	// 	struct timespec ts;

	// 	if ((error = copyin(timeout, &ts, sizeof(ts))))
	// 		return error;
	// 	if (ts.tv_sec < 0 || !timespecisvalid(&ts))
	// 		return EINVAL;
	// 	nsecs = MAX(1, MIN(TIMESPEC_TO_NSEC(&ts), MAXTSLP));
	// }

	__sync_synchronize();
	DEBUG_MSG("ADD WAIT NODE");
	add_wait_node(f, t_descr);
	__sync_synchronize();

	error = 1;

	// // TODO: Do a timed sleep or not.
	// // error = rwsleep_nsec(p, &ftlock, PWAIT|PCATCH, "fsleep", nsecs);
	// // if (error == ERESTART)
	// // 	error = ECANCELED;
	// // else if (error == EWOULDBLOCK) {
	// // 	/* A race occurred between a wakeup and a timeout. */
	// // 	if (p->p_futex == NULL)
	// // 		error = 0;
	// // 	else
	// // 		error = ETIMEDOUT;
	// // }

	// if (f->queue_dirty) {
	// 	// We've been removed -> someone wanted us to wake up
	// 	if (find_wait_node(f, t_descr) == NULL) {
	// 		f->queue_dirty = 0;
	// 		__sl_release(&global_futex_sp);
	// 		return EAGAIN;
	// 	}
	// }
	__sync_synchronize();
		__sl_release(&global_futex_sp);
		DEBUG_MSG("Going to sleep!");
		suspend(t_descr);
		__sl_acquire(&global_futex_sp);



	// // We regain control. This is either because we have been woken or because 
	// // The time timed out. We should check if we were removed from the queue and
	// // if not, we should do this on our own

	/* Remove ourself if we haven't been awaken by another thread. */
	__sync_synchronize();
	if ((find_wait_node(f, t_descr)) != NULL) {
		DEBUG_MSG("Woke up, found my node!");
		// if (-1 != remove_wait_node(f, t_descr))
			// DEBUG_MSG("Really did remove the node...");

		__sync_synchronize();
		__sl_release(&global_futex_sp);
		return EAGAIN;
	}
	__sync_synchronize();
	__sl_release(&global_futex_sp);
	return error;
}

/*
 * Wakeup at most ``n'' sibling threads sleeping on a futex at address
 * ``uaddr'' and requeue at most ``m'' sibling threads on a futex at
 * address ``uaddr2''.
 */
unsigned long 
futex_requeue(uint64_t *uaddr, uint64_t n, uint64_t *uaddr2, uint64_t m,
	int flags)
{
	__sl_acquire(&global_futex_sp);
	DEBUG_MSG("futex_requeue");
	struct futex *f = NULL, *g = NULL;
	struct wait_node *current;
	unsigned long count = 0;

	__sync_synchronize();
	__sync_synchronize();
	find_futex(global_futex_list, &f, uaddr);

	if (f == NULL) {
		// printf("Wake. FUTEX NULL!\n");
		__sl_release(&global_futex_sp);
		DEBUG_MSG("Futex does not exist; Abort requeue");
		return 0;
	}

	// current = f->sleep_queue_head;
	// if (NULL != current) {
	// 	remove_wait_node(f, current->thr);
	// 	++count;
	// }

	__sync_synchronize();
	current = f->sleep_queue_head;
	// uint64_t val2 = __atomic_load_n(uaddr, __ATOMIC_SEQ_CST); 
	// if ((current == NULL) ) {
	// 	printf("Empty queue!\n");
	// 	if(val2 != 0)
	// 		printf("But waiters...\n");
	// }
	DEBUG_MSG("Waking threads...");
	struct wait_node* head = NULL; 
	while ((NULL != f->sleep_queue_head) /*&& (count < (n + m))*/) {
		head = pop_wait_node(f);
		if (1/*count < n*/) {
			// printf("count=%d\n", count);
			// printf("Pointer of current %p\n", current);
			pthread_descr thread = head->thr; 
			// printf("Derefed, tID=%llu\n", thread);
			// next = current->next;
			// printf("Pointer of next %p\n", next);
			DEBUG_MSG("Remove node");
			// printf("Wake...%lu\n", thread);
			__sync_synchronize();
			restart(thread);
			__sync_synchronize();
			// printf("Did restart...%lu\n", thread);

			// if(next == current) {
			// 	DEBUG_MSG("Just remvoed the head. Better we stop");
			// 	break;
			// }
		} else if (uaddr2 != NULL) {
			DEBUG_MSG("Requeue to other futex");
			add_futex_node(&global_futex_list, &g, uaddr2);
			add_wait_node(g, current->thr);
		}
		// current = next;
		++count;
		// if (downs != ups)
		// 	printf("ups != downs\n");
	}
	__sync_synchronize();
	__sl_release(&global_futex_sp);
	return count;
}

/*
 * Wakeup at most ``n'' sibling threads sleeping on a futex at address
 * ``uaddr''.
 */
int
futex_wake(uint64_t *uaddr, uint64_t n, int flags)
{
	return futex_requeue(uaddr, n, NULL, 0, flags);
}