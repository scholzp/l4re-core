# Makefile to include to set the right variables from config choices for pthreads.
PKGDIR ?= ..

# just define some names for the respective pthreads directory names.
PTH_DEFAULT = libpthread
PTH_SEMAPHORE = libpthread-semaphore
PTH_SPINLOCK_MOD = libpthread-spinmod
# This is the target for later use in the Makefile in the lib directory.
# It only serves the purpose for building the correct version of pthreads.
PTH_TARGET =$(PTH_DEFAULT)

# if a new path is to be added, sadly a new step must be added to this
# if cascade.
ifeq ($(CONFIG_UCLIBC_PTHREADS_DEFAULT),y)
PTH_DIR ?= $(PKGDIR)/../uclibc/lib/$(PTH_DEFAULT)
# default target for Makefile usage set a few line before
else
ifeq ($(CONFIG_UCLIBC_PTHREADS_SEMAPHORE),y)
PTH_DIR ?= $(PKGDIR)/../uclibc/lib/$(PTH_SEMAPHORE)
PTH_TARGET =$(PTH_SEMAPHORE)
else
ifeq ($(CONFIG_UCLIBC_PTHREADS_SPINLOCK_MODIFIED),y)
PTH_DIR ?= $(PKGDIR)/../uclibc/lib/$(PTH_SPINLOCK_MOD)
PTH_TARGET =$(PTH_SPINLOCK_MOD)
endif # CONFIG_UCLIBC_PTHREADS_SEMAPHORE
endif # CONFIG_UCLIBC_PTHREADS_SEMAPHORE
endif # CONFIG_UCLIBC_PTHREADS_DEFAULT