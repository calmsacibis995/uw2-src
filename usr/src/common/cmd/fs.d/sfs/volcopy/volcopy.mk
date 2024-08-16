#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/volcopy/volcopy.mk	1.1.2.2"
#ident "$Header: volcopy.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/sfs
OWN = bin
GRP = bin

LDLIBS=-lgenIO
OBJS=

all:  volcopy

volcopy: volcopy.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(ROOTLIBS)

install: volcopy
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) volcopy

clean:
	-rm -f volcopy.o

clobber: clean
	rm -f volcopy
