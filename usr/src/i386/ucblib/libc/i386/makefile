#ident	"@(#)ucb:i386/ucblib/libc/i386/makefile	1.3"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#
#  makefile for ucblibc/i386
#
#

include $(LIBRULES)

MAC=
M4=m4 m4.def
PROF=
NONPROF=
SDEFLIST=
ARFLAGS = q

OBJECTS=

POBJECTS=

ALL:
	cd gen; $(MAKE) 
	cd sys; $(MAKE) 

all:	ALL

archive:
	#
	# Note that "archive" is invoked with libc/object as current directory.
	#
	$(LORDER) *.o|$(TSORT) >objlist
	$(AR) $(ARFLAGS) ../lib.libucb `cat objlist`
clean:
	-rm -f *.o

clobber: clean
	-rm -f */*.o
