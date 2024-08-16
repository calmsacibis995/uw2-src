#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/ncheck/ncheck.mk	1.3.5.3"
#ident "$Header: ncheck.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
OWN = bin
GRP = bin
OBJS=
LDLIBS = -lgen

all:  ncheck

ncheck: ncheck.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: ncheck
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ncheck

clean:
	-rm -f ncheck.o

clobber: clean
	rm -f ncheck
