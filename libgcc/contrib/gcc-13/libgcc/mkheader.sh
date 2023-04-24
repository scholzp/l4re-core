#! /bin/sh

# Copyright (C) 2001-2023 Free Software Foundation, Inc.
# This file is part of GCC.

# GCC is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.

# GCC is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with GCC; see the file COPYING3.  If not see
# <http://www.gnu.org/licenses/>.  


# Print libgcc_tm.h to the standard output.
# DEFINES and HEADERS are expected to be set in the environment.

# Add multiple inclusion protection guard, part one.
echo "#ifndef LIBGCC_TM_H"
echo "#define LIBGCC_TM_H"

# Generate the body of the file
echo "/* Automatically generated by mkheader.sh.  */"
for def in $DEFINES; do
    echo "#ifndef $def" | sed 's/=.*//'
    echo "# define $def" | sed 's/=/ /'
    echo "#endif"
done

for file in $HEADERS; do
    echo "#include \"$file\""
done

# Add multiple inclusion protection guard, part two.
echo "#endif /* LIBGCC_TM_H */"
