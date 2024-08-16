#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

yacc='/usr/bin/yacc -Sm25000'
ccflags="$ccflags -UM_I86"
d_mymalloc=define
echo "NOTE: you may have problems due to a spurious semicolon on the strerror()"
echo "macro definition in /usr/include/string.h.  If so, delete the semicolon."
