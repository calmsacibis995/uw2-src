#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mk:common/lib/zzzzz/zzzzz.mk	1.2"

# this dummy lib should be the last lib built.
# if the sentinel zzzzz placed in $(USRLIB) does not exist,
# we know that all libs did not build from .lib.mk

include $(LIBRULES)


all clean clobber:


install:
	touch $(USRLIB)/zzzzz
