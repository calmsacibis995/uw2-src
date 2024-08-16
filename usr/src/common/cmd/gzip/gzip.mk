#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1993 UNIVEL

#ident	"@(#)gzip:gzip.mk	1.1"
#ident "$Header: $"

include $(CMDRULES)


OWN = bin
GRP = bin

all: gzip

gzip:
	make -f Makefile

install: all
	 $(INS) -f $(USRBIN) -m 0555 -u $(OWN) -g $(GRP) gzip

clean:
	-make -f Makefile clean

clobber: clean
	make -f Makefile clean

lintit:
	$(LINT) $(LINTFLAGS) *.c
