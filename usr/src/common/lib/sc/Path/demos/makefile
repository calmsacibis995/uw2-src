#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sc:Path/demos/makefile	3.5"

include $(LIBRULES)
include $(SCLIB)/demorules

all:
	$(SHELL) $(SCBIN)/regress 

more.r: more.E
	rm -f more.V
	cd .; echo `type cat` > more.V
	./more.E >more.r

clean:
	$(CLEAN_DEMOS)

clobber:
	clean

