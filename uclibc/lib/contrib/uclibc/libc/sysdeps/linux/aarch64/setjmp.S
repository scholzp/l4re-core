/* setjmp for ARM.
   Copyright (C) 1997, 1998 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#include <features.h>
#include "jmpbuf-offsets.h"

.global __sigsetjmp
.type __sigsetjmp,%function

__sigsetjmp:
	stp	x19, x20, [x0, #JB_X19<<3]
	stp	x21, x22, [x0, #JB_X21<<3]
	stp	x23, x24, [x0, #JB_X23<<3]
	stp	x25, x26, [x0, #JB_X25<<3]
	stp	x27, x28, [x0, #JB_X27<<3]
	stp	x29, x30, [x0, #JB_X29<<3]
#if defined __UCLIBC_HAS_FLOATS__ && ! defined __UCLIBC_HAS_SOFT_FLOAT__ && ! defined BID_VARIANT_FLAG_NOFPU
	stp	 d8,  d9, [x0, #JB_D8<<3]
	stp	d10, d11, [x0, #JB_D10<<3]
	stp	d12, d13, [x0, #JB_D12<<3]
	stp	d14, d15, [x0, #JB_D14<<3]
#endif
	mov	x2,  sp
	str	x2,  [x0, #JB_SP<<3]
#ifdef IS_IN_rtld
	/* In ld.so we never save the signal mask */
	mov	w0, #0
	ret
#else
	b	__sigjmp_save
#endif
