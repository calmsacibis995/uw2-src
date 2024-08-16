#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/look/look.mk	1.4"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


#     Makefile for look

include $(CMDRULES)

INSDIR = $(USR)/ucb

OWN = bin

GRP = bin

LIBDIR = $(USR)/ucblib

MAKEFILE = look.mk

MAINS = look

OBJECTS =  look.o

SOURCES = look.c 

ALL:          $(MAINS)

$(MAINS):	look.o
	$(CC) -o look look.o $(LDFLAGS) $(PERFLIBS)
	
look.o:		$(INC)/stdio.h \
		$(INC)/ctype.h 

GLOBALINCS = $(INC)/stdio.h \
	$(INC)/ctype.h 

clean:
	rm -f $(OBJECTS)

clobber:
	rm -f $(OBJECTS) $(MAINS)

all :	ALL

install:	ALL
	$(INS) -f $(INSDIR) -u $(OWN) -g $(GRP) -m 00555 $(MAINS)
	[ -d $(LIBDIR)/dict ] || mkdir -p $(LIBDIR)/dict
	$(CH)-chown $(OWN) $(LIBDIR)/dict
	$(CH)-chgrp $(GRP) $(LIBDIR)/dict
	$(CH)-chmod 755 $(LIBDIR)/dict
	$(INS) -f $(LIBDIR)/dict -u $(OWN) -g $(GRP) -m 0444 words

