#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/tunefs/tunefs.mk	1.5.6.3"
#ident "$Header: tunefs.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

OBJS=

all:  tunefs

tunefs: tunefs.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: tunefs
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) tunefs
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) tunefs
	
clean:
	-rm -f tunefs.o

clobber: clean
	rm -f tunefs
