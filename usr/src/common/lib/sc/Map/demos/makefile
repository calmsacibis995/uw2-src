#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:Map/demos/makefile	3.3"

include $(LIBRULES)
include $(SCLIB)/demorules

all:
	$(SHELL) $(SCBIN)/regress 

example.r:	example.E
	cat example.c | ./example.E >example.r

topsort.r:	topsort.E
	cat clothes | ./topsort.E >topsort.r

clean:
	$(CLEAN_DEMOS)

clobber:
	clean

