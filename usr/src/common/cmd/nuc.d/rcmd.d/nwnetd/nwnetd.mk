#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rcmd.cmd:nwnetd/nwnetd.mk	1.5"
#ident  "$Header: $"

include $(CMDRULES)

#
OWN=root
GRP=sys

#	Where MAINS are to be installed.
INSDIR = $(USRSBIN)

LIB=-lgen -lxchoose -lnsl -lsocket 

LDLIBS=$(LIB)

all: nwnetd

nwnetd: nwnetd.o daemon.o
	$(CC) -o nwnetd nwnetd.o daemon.o $(LDFLAGS) $(LDLIBS)

install: nwnetd nwnetdmsgs.str $(DIRS)
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	-rm -f $(INSDIR)/nwnetd
	$(INS) -f $(INSDIR) -m 0755 -u bin -g bin nwnetd
	[ -d $(ETC) ] || mkdir -p $(ETC)
	-rm -f $(ETC)/nwnetd.conf
	$(INS) -f $(ETC) -m 00644 -u $(OWN) -g $(GRP) nwnetd.conf
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 nwnetdmsgs.str

clean:
	rm -f nwnetd.o
	rm -f daemon.o
	
clobber: clean
	rm -f nwnetd

lintit:
	$(LINT) $(LINTFLAGS) nwnetd.c
	$(LINT) $(LINTFLAGS) daemon.c
