#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/quotaon/quotaon.mk	1.7.5.3"
#ident "$Header: quotaon.mk 1.2 91/04/11 $"

include $(CMDRULES)

INSDIR = $(USRLIB)/fs/ufs
INSDIR2 = $(USRSBIN)
OWN =  bin
GRP = bin
OBJS=
LDLIBS = -lgen

all:  quotaon

quotaon: quotaon.o $(OBJS)
	$(CC) $(LDFLAGS) -o $@  $@.o $(OBJS) $(LDLIBS) $(SHLIBS)

install: quotaon
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/quotaon
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) quotaon
	-rm -f $(INSDIR)/quotaoff
	ln $(INSDIR)/quotaon $(INSDIR)/quotaoff
	-rm -f $(INSDIR2)/quotaon
	ln $(INSDIR)/quotaon $(INSDIR2)/quotaon
	-rm -f $(INSDIR2)/quotaoff
	ln $(INSDIR)/quotaon $(INSDIR2)/quotaoff
	
clean:
	-rm -f quotaon.o

clobber: clean
	rm -f quotaon
