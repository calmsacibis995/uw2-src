#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#ident	"@(#)ucb:common/ucbcmd/tr/tr.mk	1.3"
#ident	"$Header: $"
# 	Portions Copyright (c) 1988, Sun Microsystems, Inc.
#	All rights reserved.


#	Makefile for tr 

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = tr.mk

MAINS = tr

OBJECTS =  tr.o

SOURCES =  tr.c

ALL:	$(MAINS)

tr:	tr.o 
	$(CC) -o tr  tr.o   $(LDFLAGS) $(PERFLIBS)


tr.o:	$(INC)/stdio.h

GLOBALINCS = $(INC)/stdio.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) $(MAINS)

