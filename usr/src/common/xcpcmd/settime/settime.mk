#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)xcpsettime:settime.mk	1.2.2.1"
#ident  "$Header: settime.mk 1.2 91/07/11 $"

include $(CMDRULES)

#	Makefile for settime

all:

settime: 

install: all
	-rm -f $(USRBIN)/settime 
	ln $(USRBIN)/touch $(USRBIN)/settime 

clean clobber lintit:
