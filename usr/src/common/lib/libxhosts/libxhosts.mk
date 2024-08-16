#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libhosts:libxhosts.mk	1.1"
include $(LIBRULES)
all clean clobber install localinstall lintit:
	cd libhosts; make -f *.mk $@
	cd sharedObjects; make -f *.mk $@
