#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:lib/printers/Makefile	1.14.2.2"
#ident "$Header: Makefile 1.2 91/04/12 $"
#
# Makefile for printermgmt(lp) routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= \
		chkprinter.c \
		default.c \
		delprinter.c \
		freeprinter.c \
		getprinter.c \
		p_head.c \
		okprinter.c \
		printwheels.c \
		putprinter.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LINTS	= \
		$(LINTLP)

LIBNAME	=	lpprt

include ../libcommon.mk
