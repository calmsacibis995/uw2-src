#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:share/share.mk	1.3.5.3"
#ident "$Header: share.mk 1.3 91/04/08 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
FRC =

all: share

share: share.c 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ share.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 share

clean:
	rm -f share.o

clobber: clean
	rm -f share
FRC:
