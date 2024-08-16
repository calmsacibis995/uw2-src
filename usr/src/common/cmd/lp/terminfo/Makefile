#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp:terminfo/Makefile	1.3.5.1"
#ident	"$Header: $"

#
# Makefile for the Terminfo entries delivered with LP
#


include $(CMDRULES)

TOP	=	..

include ../common.mk


TERMINFO=	$(USRSHARELIB)/terminfo
TITMP	=	$(TERMINFO)/ti


COMPILE	=	tic -v


SRCS	= \
		PS.ti

all:
		
install:	ckdir \
		terminfo.src
	TERMINFO=$(TERMINFO) $(COMPILE) terminfo.src
	cp $(SRCS) $(TITMP)

clean:
	$(RM) terminfo.src

clobber:	clean

strip:

terminfo.src:	$(SRCS)
	@cat $(SRCS) >terminfo.src

ckdir:
	@if [ ! -d $(TERMINFO) ]; \
	then \
		mkdir $(TERMINFO); \
		$(CH)chgrp bin $(TERMINFO); \
		$(CH)chown bin $(TERMINFO); \
		chmod 775 $(TERMINFO); \
	fi
	@if [ ! -d $(TITMP) ]; \
	then \
		mkdir $(TITMP); \
		$(CH)chgrp root $(TITMP); \
		$(CH)chown root $(TITMP); \
		chmod $(DMODES) $(TITMP); \
	fi

lintit:

lintlib:
