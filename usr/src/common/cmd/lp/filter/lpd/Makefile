#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp:filter/lpd/Makefile	1.2"
#ident	"$Header: $"

#
# Makefile for lp/filter/lpd/catv.fd
# and lp/filter/lpd/prtitle.fd
#


include $(CMDRULES)

TOP	=	../..

include ../../common.mk


FDTMP	=	$(ETCLP)/fd


SRCS	=	catv.fd \
		prtitle.fd


all:		$(SRCS)

install:	ckdir
	cp $(SRCS) $(FDTMP)

ckdir:
	@if [ ! -d $(FDTMP) ]; \
	then \
		mkdir $(FDTMP); \
		$(CH)chgrp $(GROUP) $(FDTMP); \
		$(CH)chown $(OWNER) $(FDTMP); \
		chmod $(DMODES) $(FDTMP); \
	fi

lintit:

clean:

clobber:	clean

