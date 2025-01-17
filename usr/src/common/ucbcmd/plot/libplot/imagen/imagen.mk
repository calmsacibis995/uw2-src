#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/imagen/imagen.mk	1.1"
#ident	"$Header: $"

#	Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
#	All Rights Reserved.

#     Makefile for imagen

include $(CMDRULES)

DEFRES = 240

LOCALDEF = -DDEFRES=$(DEFRES)

LOCALINC = -I$(ROOT)/$(MACH)/usr/ucbinclude 

ARFLAGS = cr

MAKEFILE = imagen.mk

MAINS = ../libimagen.a

OBJECTS = arc.o box.o charset.o circle.o close.o cont.o dot.o erase.o label.o \
        line.o linemod.o move.o open.o point.o scale.o space.o 

SOURCES = arc.c box.c charset.c circle.c close.c cont.c dot.c erase.c label.c \
        line.c linemod.c move.c open.c point.c scale.c space.c 

ALL:          $(MAINS)

$(MAINS):	$(OBJECTS)	
	$(AR) $(ARFLAGS) $(MAINS) `$(LORDER) $(OBJECTS) | $(TSORT)`
	
clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(MAINS)


