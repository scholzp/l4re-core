PKGDIR	?= ..
L4DIR	?= $(PKGDIR)/../../..

include $(L4DIR)/mk/Makeconf
# Inlude choice specfic pthreads variables.
include $(PKGDIR)/../uclibc/pthread.mk

TARGET += $(PTH_TARGET) uclibc minimal_l4re

include $(L4DIR)/mk/subdir.mk

uclibc minimal_l4re: libpthread

