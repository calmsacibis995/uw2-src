#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/mount/mount.mk	1.3.5.5"
#ident "$Header: mount.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

OBJS =
all:  mount

mount: mount.o
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy $@.o $(OBJS) $(LDLIBS) $(ROOTLIBS)

install: mount
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mount.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mount.dy

clean:
	-rm -f mount.o

clobber: clean
	rm -f mount mount.dy
