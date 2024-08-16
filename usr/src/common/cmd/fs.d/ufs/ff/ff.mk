#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/ff/ff.mk	1.3.5.3"
#ident "$Header: ff.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR1 = $(USRLIB)/fs/ufs
OWN = bin
GRP = bin
LDLIBS = -lgen

all:  ff

ff: ff.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR1) ] || mkdir -p $(INSDIR1)
	$(INS) -f $(INSDIR1) -m 0555 -u $(OWN) -g $(GRP) ff

clean:
	-rm -f ff.o

clobber: clean
	rm -f ff
