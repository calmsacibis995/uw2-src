#ident	"@(#)ucb:i386/ucblib/libc/i386/sys/makefile	1.4"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#
#
# makefile for ucblib/libc/i386/sys
#

include $(LIBRULES)

CFLAGS=-c

OBJECTS= syscall.o _times.o gettimeofday.o

SOURCES= syscall.s _times.s gettimeofday.s

.s.o:
	$(CC) $(CFLAGS) $*.s

ALL:	$(OBJECTS)

all:	ALL

clean:
	rm -rf *.o

clobber:
	rm -rf *.o
