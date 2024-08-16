#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

optimize='-O1'
d_mymalloc=define
mallocsrc='malloc.c'
mallocobj='malloc.o'
d_voidsig=define
d_vfork=undef
d_charsprf=undef
case `(uname -r) 2>/dev/null` in
4*)libswanted=`echo $libswanted | sed 's/c_s \(.*\)/\1 c_s/'`
    ccflags="$ccflags -DLANGUAGE_C -DBSD_SIGNALS -cckr -signed"
    ;;
esac
