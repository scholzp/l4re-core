PKGDIR ?= ..

PTH_STOCK = libpthread
PTH_LLFUTEX = libpthread-llfutex-port

ifeq ($(CONFIG_UCLIBC_STOCK_TRACING),y)
PTH_DIR ?= $(PKGDIR)/../uclibc/lib/$(PTH_LLFUTEX)
else
PTH_DIR ?= $(PKGDIR)/../uclibc/lib/$(PTH_STOCK)
endif