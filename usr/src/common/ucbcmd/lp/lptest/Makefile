#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/lp/lptest/Makefile	1.2"
#ident	"$Header: $"

include $(CMDRULES)

#TOP	=	../../..

include ../common.mk

TAGS	=	ctags

USRUCB	=	$(ROOT)/$(MACH)/usr/ucb

LOCALDEF = $(DEBUG)

LOCALINC = -I$(LPINC) 

SRCS	= \
		lptest.c

OBJS	=	$(SRCS:.c=.o)


CMD	=	lptest


all:	$(CMD)

install:	all
	$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(USRUCB) $(CMD)

clean:
	$(RM) $(OBJS)

clobber:	clean
	$(RM) $(CMD)

strip:
	$(STRIP) $(CMD)

stamp:

lint:
	$(LINT) $(CFLAGS) $(SRCS)

ctags:
	$(TAGS) $(SRCS)
