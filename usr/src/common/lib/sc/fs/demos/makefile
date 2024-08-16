#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:fs/demos/makefile	3.9"

include $(LIBRULES)
OTHERCCFLAGS=-f
include $(SCLIB)/demorules

.SUFFIXES: .r .E

all:
	$(SHELL) $(SCBIN)/regress

.c.r:
	$(C++CMD) $(LDFLAGS) -o $*.E $< -lfs $(ARCHIVES)
	@-./$*.E 2>&1 | sed -e 's/at [0123456789abcdef]*)/at -----)/' > $@

.c.E:
	$(C++CMD) $(LDFLAGS) -o $*.E $< -lfs $(ARCHIVES)

.c.o:
	$(C++CMD) -c $<

clean:
	$(CLEAN_DEMOS)

clobber:
	clean

