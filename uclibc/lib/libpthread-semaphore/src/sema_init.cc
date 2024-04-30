#include "sema_init.h"
#include <l4/sys/thread.h>
#include <l4/util/util.h>
#include <l4/re/env>
#include <l4/sys/semaphore>
#include <l4/re/util/cap_alloc>
#include <l4/re/util/unique_cap>
#include <l4/sys/ipc.h>
#include <l4/sys/utcb.h>
#include <l4/sys/capability>
#include <l4/sys/thread>
#include <l4/re/env>
#include <l4/sys/factory>
#include <l4/re/util/cap_alloc>

l4_cap_idx_t acquire_l4_semaphore_cap() {
    L4Re::Env *env = const_cast<L4Re::Env*>(L4Re::Env::env());
    L4::Cap<L4::Semaphore> s(env->first_free_cap() << L4_CAP_SHIFT);
    return s.cap();
}