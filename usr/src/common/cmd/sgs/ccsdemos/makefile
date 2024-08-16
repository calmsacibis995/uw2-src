#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ccsdemos:makefile	1.4"

# This is an install-only makefile.  

include $(CMDRULES)

# This should contain the names of all subdirectories
DEMOS = thr_demos sc_demos

INSDIR = $(CCSBIN)/../demos

all:

install: $(INSDIR) 
	cp -r $(DEMOS) $(INSDIR)

clean:

clobber:
	$(RM) -rf $(INSDIR)

$(INSDIR) :
	mkdir -p $(@)
