#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rcmd.cmd:nrexecd/nrexecd.mk	1.9"
#ident  "$Header: $"

include $(CMDRULES)

OWN=bin
GRP=bin

#	Where MAINS are to be installed.
INSDIR = $(USRSBIN)

LIB=-lxchoose -lnsl -lNwClnt -lnwutil -lNwCal -lNwNcp -lNwLoc -lgen

LDLIBS=$(LIB)

all: nrexecd

nrexecd: nrexecd.o 
	$(CC) -o nrexecd nrexecd.o $(LDFLAGS) $(LDLIBS)

install: nrexecd nrexecdmsgs.str
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	-rm -f $(INSDIR)/nrexecd
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) nrexecd
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 nrexecdmsgs.str
	
clean:
	rm -f nrexecd.o
	
clobber: clean
	rm -f nrexecd

lintit:
	$(LINT) $(LINTFLAGS) nrexecd.c
