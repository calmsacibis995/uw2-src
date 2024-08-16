#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)sfs.cmds:common/cmd/fs.d/sfs/ncheck/ncheck.mk	1.2.4.3"
#ident "$Header: ncheck.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/sfs
OWN = bin
GRP = bin
OBJS=
LDLIBS = -lgen

all:  ncheck

ncheck: ncheck.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(ROOTLIBS)

ncheck.o: ncheck.c \
	$(INC)/stdio.h \
	$(INC)/sys/param.h \
	$(INC)/sys/types.h \
	$(INC)/sys/vnode.h \
	$(INC)/sys/acl.h \
	$(INC)/sys/fs/sfs_inode.h \
	$(INC)/sys/fs/sfs_fs.h \
	$(INC)/sys/fs/sfs_fsdir.h

install: ncheck
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ncheck

clean:
	-rm -f ncheck.o

clobber: clean
	rm -f ncheck
