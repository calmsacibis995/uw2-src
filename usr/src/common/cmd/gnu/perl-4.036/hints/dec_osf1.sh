#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

d_crypt='undef'             # The function is there, but it is empty
d_odbm='undef'              # We don't need both odbm and ndbm
gidtype='gid_t'
groupstype='int'
libpth="$libpth /usr/shlib" # Use the shared libraries if possible
libc='/usr/shlib/libc.so'   # The archive version is /lib/libc.a
case `uname -m` in
    mips|alpha)   optimize="$optimize -O2 -Olimit 2900"
                  ccflags="$ccflags -std1 -D_BSD" ;;
    *)            ccflags="$ccflags -D_BSD" ;;
esac
