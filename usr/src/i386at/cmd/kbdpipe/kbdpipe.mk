#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kbdpipe:kbdpipe.mk	1.2.1.3"
#ident "$Header: "

include $(CMDRULES)

all: kbdpipe

kbdpipe: kbdpipe.o
	$(CC) kbdpipe.o -o $@ $(LDFLAGS)

install:	all
	$(INS) -f $(USRBIN) kbdpipe

clean:
	rm -f *.o

clobber:	clean
	rm -f kbdpipe
