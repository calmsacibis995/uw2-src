#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ps:ps.mk	1.7.25.4"
#
# makefile for ps(1) command
#

include $(CMDRULES)

PROCISSUE = PROC Issue 2 Version 1

LDLIBS = -s -lcmd -lw -lgen

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

LINTFLAGS=$(DEFLIST)

all:	ps

install:	ps
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ps

ps: ps.c
	$(CC) $(CFLAGS) -o ps ps.c $(LDFLAGS) $(LDLIBS) $(NOSHLIBS)

lintit:
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	rm -f *.o

clobber:	clean
	rm -f ps
