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

#ident	"@(#)ucb:common/ucbcmd/expand/expand.mk	1.2"
#ident	"$Header: $"

#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#	Makefile for expand and unexpand

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = expand.mk

MAINS = expand unexpand

OBJECTS =  expand.o unexpand.o

SOURCES =  expand.c unexpand.c

ALL:		$(MAINS)

expand:		expand.o 
	$(CC) -o expand  expand.o   $(LDFLAGS) $(PERFLIBS)

expand.o:	 $(INC)/stdio.h

unexpand:	unexpand.o
	$(CC) -o unexpand  unexpand.o   $(LDFLAGS) $(PERFLIBS)

unexpand.o:	$(INC)/stdio.h

GLOBALINCS = $(INC)/stdio.h


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -u $(OWN) -g $(GRP) -m 0555 -f $(INSDIR) expand
	$(INS) -u $(OWN) -g $(GRP) -m 0555 -f $(INSDIR) unexpand

