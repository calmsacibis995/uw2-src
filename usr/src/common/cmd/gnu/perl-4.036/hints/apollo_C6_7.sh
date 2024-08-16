#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

optimize='-opt 2'
cflags='-A nansi cpu,mathchip -O -U__STDC__'
echo "Some tests may fail unless you use 'chacl -B'.  Also, op/stat"
echo "test 2 may fail because Apollo doesn't support mtime or ctime."
