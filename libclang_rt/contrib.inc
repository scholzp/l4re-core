include $(L4DIR)/mk/Makeconf

LIBCLANG_RT_CONTRIB_MIN_VERSION := 15
LIBCLANG_RT_CONTRIB_MAX_VERSION := 16
LIBCLANG_RT_CONTRIB_SUPPORTED_VERSIONS := $(shell seq $(LIBCLANG_RT_CONTRIB_MIN_VERSION) $(LIBCLANG_RT_CONTRIB_MAX_VERSION))
LIBCLANG_RT_CONTRIB_VERSION := $(GCCVERSION)

ifeq ($(filter $(LIBCLANG_RT_CONTRIB_VERSION),$(LIBCLANG_RT_CONTRIB_SUPPORTED_VERSIONS)),)
$(warning No suitable compiler-rt version found for clang $(LIBCLANG_RT_CONTRIB_VERSION).)
$(warning Falling back to compiler-rt $(LIBCLANG_RT_CONTRIB_MAX_VERSION), unexpected errors may occur.)
$(warning Please update the compiler-rt package.)
LIBCLANG_RT_CONTRIB_VERSION := $(LIBCLANG_RT_CONTRIB_MAX_VERSION)
endif

LIBCLANG_RT_CONTRIB_VERSIONS := $(shell seq $(LIBCLANG_RT_CONTRIB_VERSION) -1 $(LIBCLANG_RT_CONTRIB_MIN_VERSION))
LIBCLANG_RT_CONTRIB_SUFFIX_VERSIONS := $(addprefix -,$(LIBCLANG_RT_CONTRIB_VERSIONS))

LIBCLANG_RT_CONTRIB_DIR = $(LIBCLANG_RT_PKG_DIR)/contrib/llvm-$(LIBCLANG_RT_CONTRIB_VERSION)
LIBCLANG_RT_CONTRIB_SRC_DIR = $(LIBCLANG_RT_CONTRIB_DIR)/compiler-rt

CPU_TYPE_x86   := i386
CPU_TYPE_amd64 := i386
CPU_TYPE_arm   := arm
CPU_TYPE_arm64 := aarch64
CPU_TYPE_mips  := mips
CPU_TYPE_riscv := riscv
CPU_TYPE        = $(CPU_TYPE_$(BUILD_ARCH))