#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	copyright	"1"

#ident	"@(#)maplang:maplang.mk	1.1"
#	Makefile for maplang

include $(CMDRULES)

INSDIR = $(USRSADM)/install/bin

MAINS = maplang

SOURCES =  maplang.sh

all:	$(MAINS)

maplang:	 $(SOURCES)
	cp maplang.sh maplang

clean:
	rm -f $(MAINS)

clobber: clean

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 $(MAINS)

