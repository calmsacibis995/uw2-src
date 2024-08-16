#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/lastcomm/lastcomm.mk	1.1.1.1"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


#     Makefile for lastcomm

include $(CMDRULES)

LDFLAGS = -lgen
INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

MAKEFILE = lastcomm.mk

MAINS = lastcomm

OBJECTS =  lastcomm.o

SOURCES = lastcomm.c 

ALL:          $(MAINS)

$(MAINS):	lastcomm.o
	$(CC) -o lastcomm lastcomm.o $(LDFLAGS) $(PERFLIBS)
	
lastcomm.o:		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/pwd.h \
		$(INC)/sys/stat.h \
		$(INC)/utmp.h \
		$(INC)/ctype.h \
		$(INC)/sys/param.h \
		$(INC)/sys/acct.h \
		$(INC)/sys/file.h 


GLOBALINCS = $(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/pwd.h \
		$(INC)/sys/stat.h \
		$(INC)/utmp.h \
		$(INC)/ctype.h \
		$(INC)/sys/param.h \
		$(INC)/sys/acct.h \
		$(INC)/sys/file.h 


clean:
	rm -f $(OBJECTS)

clobber:	clean
	rm -f $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -m 00555 -u $(OWN) -g $(GRP) lastcomm 

