#ident	"@(#)ucb:common/ucbcmd/ls/ls.mk	1.3"
#ident	"$Header: $"
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

#	Makefile for ls

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucb

LDLIBS = -lgen 

MAKEFILE = ls.mk

MAINS = ls

OBJECTS =  ls.o

SOURCES =  ls.c

ALL:		$(MAINS)

ls:	ls.o
	$(CC) -o ls ls.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

ls.o:		 $(INC)/sys/param.h \
		 $(INC)/sys/stat.h \
		 $(INC)/sys/sysmacros.h \
		 $(INC)/stdio.h \
		 $(INC)/dirent.h \
		 $(INC)/errno.h	\
		 $(INC)/sys/types.h \
		 $(INC)/sys/termios.h

GLOBALINCS = $(INC)/stdio.h \
	$(INC)/dirent.h \
	$(INC)/sys/param.h \
	$(INC)/sys/stat.h \
	$(INC)/sys/sysmacros.h \
	$(INC)/sys/termios.h \
	$(INC)/errno.h	\
	$(INC)/sys/types.h 


clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all : ALL

install: ALL
	$(INS) -f $(INSDIR) $(MAINS)

