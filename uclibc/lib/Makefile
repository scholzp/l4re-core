PKGDIR	?= ..
L4DIR	?= $(PKGDIR)/../../..

include $(L4DIR)/mk/Makeconf
include $(PKGDIR)/../uclibc/pthread.mk

ifeq ($(CONFIG_UCLIBC_STOCK_TRACING),y)
TARGET += $(PTH_W_TRACES)
else
TARGET += $(PTH_STOCK)
endif
TARGET += uclibc minimal_l4re

include $(L4DIR)/mk/subdir.mk

uclibc minimal_l4re: libpthread

