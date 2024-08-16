#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/checkeq/checkeq.mk	1.1"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


#     Makefile for checkeq

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = checkeq.mk

MAINS = checkeq

OBJECTS =  checkeq.o

SOURCES = checkeq.c 

ALL:          $(MAINS)

$(MAINS):	checkeq.o
	$(CC) -o checkeq checkeq.o $(LDFLAGS) $(PERFLIBS)
	
checkeq.o:		$(INC)/stdio.h 

GLOBALINCS = $(INC)/stdio.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 $(MAINS)

