#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:general/general.mk	1.2.5.3"
#ident "$Header: general.mk 1.4 91/04/08 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
FRC =

all: general

general: general.c 
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ general.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 general

clean:
	rm -f general.o

clobber: clean
	rm -f general
FRC:
