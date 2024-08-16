#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/quot/quot.mk	1.5.5.3"
#ident "$Header: quot.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
OBJS=
LDLIBS = -lgen

all:  quot

quot: quot.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: quot
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) quot
	-rm -f $(INSDIR2)/quot
	ln $(INSDIR)/quot $(INSDIR2)/quot
	
clean:
	-rm -f quot.o

clobber: clean
	rm -f quot
