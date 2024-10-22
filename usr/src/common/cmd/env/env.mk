#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)env:env.mk	1.4.3.2"
#ident "$Header: env.mk 1.2 91/04/08 $"
#	env make file

include $(CMDRULES)

INSDIR = $(USRBIN)
OWN = bin
GRP = bin

SOURCE = env.c

all:	env

env:	$(SOURCE)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ env.c $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: env
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) env

clean:
	rm -f env.o

clobber:	clean
	  rm -f env
