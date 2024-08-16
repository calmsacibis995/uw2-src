#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

: Just disable defaulting to -fpcc-struct-return, since gcc is native compiler.
nativegcc='define'
groupstype="int"
usemymalloc="n"
libswanted='dbm sys_s'
