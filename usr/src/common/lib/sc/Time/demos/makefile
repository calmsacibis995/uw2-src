#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:Time/demos/makefile	3.5"

include $(LIBRULES)
include $(SCLIB)/demorules

.c.r:
	$(C++CMD) -o $*.E $< $(ARCHIVES)
	cd .; TZ=EST5EDT; export TZ; echo "12:42 noon foo" | ./$*.E > $@

all:
	$(SHELL) $(SCBIN)/regress 

cal.r:	
	$(C++CMD) -o cal.E cal.c $(ARCHIVES)
	cd .; TZ=EST5EDT; export TZ; ./cal.E < appts >cal.r 2>&1

clean:
	$(CLEAN_DEMOS)

clobber:
	clean

