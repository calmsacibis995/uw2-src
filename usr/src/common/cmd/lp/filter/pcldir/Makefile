#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp:filter/pcldir/Makefile	1.1"
#ident	"$Header: $"

#
# Makefile for lp/filter/pcl/pcl.fd
#


include $(CMDRULES)

TOP	=	../..

include ../../common.mk


FDTMP	=	$(ETCLP)/fd


SRCS	= \
		pcl.fd


all:

install:	ckdir
	cp $(SRCS) $(FDTMP)

clean:

clobber:	clean

strip:

ckdir:
	@if [ ! -d $(FDTMP) ]; \
	then \
		mkdir $(FDTMP); \
		$(CH)chgrp $(GROUP) $(FDTMP); \
		$(CH)chown $(OWNER) $(FDTMP); \
		chmod $(DMODES) $(FDTMP); \
	fi

lintit:

