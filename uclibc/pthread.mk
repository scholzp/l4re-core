PKGDIR ?= ..

PTH_STOCK = libpthread
PTH_W_TRACES = libpthread-perftrace

ifeq ($(CONFIG_UCLIBC_STOCK_TRACING),y)
PTH_DIR ?= $(PKGDIR)/../uclibc/lib/$(PTH_W_TRACES)
else
PTH_DIR ?= $(PKGDIR)/../uclibc/lib/$(PTH_STOCK)
endif
