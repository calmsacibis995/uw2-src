#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:lib/bsd/Makefile	1.8.4.1"
#ident "$Header: Makefile 1.3 91/04/16 $"
#
# Makefile for bsd routines
#

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)
#LOCALDEF= -DDEBUG

SRCS	= \
		displayq.c \
		escape.c \
		fatalmsg.c \
		findfld.c \
		gethostnm.c \
		global.c \
		log.c \
		parse.c \
		psfile.c \
		rmjob.c \
		rsendjob.c \
		sndrcv.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LIBNAME	=	lpbsd

include ../libcommon.mk
