#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)lp:lib/nuc/Makefile	1.1.1.1"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/lp/lib/nuc/Makefile,v 1.3 1994/03/26 01:48:50 aclark Exp $"
#
# Makefile for bsd routines
#

LOCALDEF = -DMCPOWERS -DN_PLAT_UNIX

include $(CMDRULES)

TOP	=	../..

include ../../common.mk

LOCALINC= -I$(LPINC)

SRCS	= 	NWcpsOps.c

LINTTXT =	$(SRCS)

OBJS	=	$(SRCS:.c=.o)

LIBNAME	=	lpnw

include ../libcommon.mk
