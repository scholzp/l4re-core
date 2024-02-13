PKGDIR ?= ..

PTH_STOCK = libpthread_perftrace
PTH_W_TRACES = libpthread_perftrace


ifeq ($(CONFIG_UCLIBC_STOCK_TRACING),y)
PTH_DIR := $(PKGDIR)/../uclibc/lib/$(PTH_W_TRACES)
else
PTH_DIR := $(PKGDIR)/../uclibc/lib/$(PTH_STOCK)
endif
