#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/mkfs/mkfs.mk	1.4.5.5"
#ident "$Header: mkfs.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/ufs
INSDIR2 = $(ETC)/fs/ufs
OWN = bin
GRP = bin
LOCALDEF = -D_KMEMUSER
LDLIBS = -lgen

all:  mkfs

mkfs: mkfs.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(ROOTLIBS)
	$(CC) $(LDFLAGS) -o $@.dy $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: mkfs
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	[ -d $(INSDIR2) ] || mkdir -p $(INSDIR2)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mkfs
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) mkfs.dy
	$(INS) -f $(INSDIR2) -m 0555 -u $(OWN) -g $(GRP) mkfs.dy

clean:
	-rm -f mkfs.o

clobber: clean
	rm -f mkfs mkfs.dy
