#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)langsup:i386/ls/cmd/loadfont/Makefile	1.1"

include $(CMDRULES)

all: loadfont

clean:
	-rm -f loadfont

clobber: 	clean

install:	all
	$(INS) -f $(ROOT)/$(MACH)/sbin -m 555 -u bin -g bin loadfont
