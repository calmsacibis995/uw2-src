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

#ident	"@(#)unlink:unlink.mk	1.2.4.1"
#ident "$Header: unlink.mk 1.3 91/03/19 $"

include $(CMDRULES)

#	Makefile for unlink

OWN = root
GRP = bin

all: unlink

unlink: unlink.c
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $@.c  $(LDFLAGS) $(LDLIBS) $(SHLIBS)

install: all
	-rm -f $(ETC)/unlink
	 $(INS) -f $(USRSBIN) -m 0500 -u $(OWN) -g $(GRP) unlink
	-$(SYMLINK) /usr/sbin/unlink $(ETC)/unlink

clean:
	rm -f unlink.o

clobber: clean
	rm -f unlink

lintit:
	$(LINT) $(LINTFLAGS) unlink.c
