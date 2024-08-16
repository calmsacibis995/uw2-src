#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/quotacheck/quotacheck.mk	1.5.5.3"
#ident "$Header: quotacheck.mk 1.2 91/04/11 $"

include $(CMDRULES)

INCSYS = $(INC)
INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN = bin
GRP = bin
LDLIBS = -lgen

OBJS=

all:  quotacheck

quotacheck: quotacheck.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: quotacheck
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) quotacheck
	-rm -f $(INSDIR2)/quotacheck
	ln $(INSDIR)/quotacheck $(INSDIR2)/quotacheck
	
clean:
	-rm -f quotacheck.o

clobber: clean
	rm -f quotacheck
