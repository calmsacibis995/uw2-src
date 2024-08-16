#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/which/which.mk	1.2"
#ident	"$Header: $"
#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#



#	which make file

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = which.mk

MAINS = which

OBJECTS =  which

SOURCES =  which.csh

ALL:		$(MAINS)

which:	 
	cp which.csh which

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS)


all : ALL

install: ALL
	$(INS) -f $(INSDIR)  -m 0555 -u $(OWN) -g $(GRP) $(MAINS)

