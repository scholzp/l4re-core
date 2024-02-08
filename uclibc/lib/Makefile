PKGDIR	?= ..
L4DIR	?= $(PKGDIR)/../../..


ifeq ($(CONFIG_UCLIBC_STOCK),y)
TARGET += libpthread
else ifeq ($(CONFIG_UCLIBC_STOCK_TRACING),y)
TARGET += libpthread_perftrace
else
TARGET += libpthread
endif
TARGET += uclibc minimal_l4re

include $(L4DIR)/mk/subdir.mk
