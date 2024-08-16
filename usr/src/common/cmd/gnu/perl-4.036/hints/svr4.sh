#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

cc='/bin/cc'
test -f $cc || cc='/usr/ccs/bin/cc'
ldflags='-L/usr/ucblib'
mansrc='/usr/share/man/man1'
ccflags='-I/usr/include -I/usr/ucbinclude'
libswanted=`echo $libswanted | sed 's/ ucb/ c ucb/'`
