#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/t300s/t300s.mk	1.1"
#ident	"$Header: $"

#	Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#     Makefile for t300s

include $(CMDRULES)

LOCALINC = -I$(ROOT)/$(MACH)/usr/ucbinclude 

ARFLAGS = cr

MAKEFILE = t300s.mk

MAINS = ../libt300s.a

OBJECTS = arc.o box.o circle.o close.o dot.o erase.o label.o \
        line.o linmod.o move.o open.o point.o space.o subr.o

SOURCES = arc.c box.c circle.c close.c dot.c erase.c label.c \
        line.c linmod.c move.c open.c point.c space.c subr.c

ALL:          $(MAINS)

$(MAINS):	$(OBJECTS)	
	$(AR) $(ARFLAGS) $(MAINS) `$(LORDER) $(OBJECTS) | $(TSORT)`
	
clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)


