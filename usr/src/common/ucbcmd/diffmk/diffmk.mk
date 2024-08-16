#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/diffmk/diffmk.mk	1.1"
#ident	"$Header: $"
#       Portions Copyright(c) 1988, Sun Microsystems, Inc.
#       All Rights Reserved



#	diffmk make file

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = diffmk.mk

MAINS = diffmk

OBJECTS =  diffmk

SOURCES =  diffmk.sh

ALL:		$(MAINS)

diffmk:	
	cp diffmk.sh diffmk

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS)


all : ALL

install: ALL
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

