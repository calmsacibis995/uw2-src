#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/ufsrestore/ufsrestore.mk	1.10.6.4"
#ident "$Header: ufsrestore.mk 1.3 91/04/17 $"

include $(CMDRULES)

# LOCALDEF:
#       DEBUG                   use local directory to find ddate and dumpdates
#       TDEBUG                  trace out the process forking
#
MAINS = ufsrestore ufsrestore.stat

OBJS= dirs.o interactive.o main.o restore.o symtab.o \
	tape.o utilities.o
SRCS= dirs.c interactive.c main.c restore.c symtab.c \
	tape.c utilities.c
HDRS= dump.h

INSDIR = $(USRLIB)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

RM= rm -f


all: $(MAINS)

ufsrestore: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(SHLIBS)

ufsrestore.stat: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) $(NOSHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ufsrestore
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ufsrestore.stat
	$(INS) -f $(USRSBIN) -m 0555 -u $(OWN) -g $(GRP) ufsrestore
	
clean:
	$(RM) $(BINS) $(OBJS)

clobber: clean
	$(RM) ufsrestore
