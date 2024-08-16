#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kbdload:kbdload.mk	1.1.1.2"
#ident "$Header: "

include $(CMDRULES)

OFILES	= loader.o main.o scrutiny.o util.o
MAIN	= kbdload


all:	$(MAIN)

kbdload: $(OFILES)
	$(CC) $(OFILES) -o $(MAIN) $(LDFLAGS)

install:	all
	$(INS) -f $(USRBIN) $(MAIN)

clean:
	rm -f *.o

clobber:	clean
	rm -f $(MAIN)
