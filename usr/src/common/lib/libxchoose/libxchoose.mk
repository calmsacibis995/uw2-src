#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libxchoose:libxchoose.mk	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libxchoose/libxchoose.mk,v 1.2 1994/05/19 18:14:15 plc Exp $"

include $(LIBRULES)

DOTA = libxchoose.a
INSDIR = $(USRLIB)

OBJS = \
	util.o \
	async.o \
	rcmd.o \
	rexec.o 

all:	$(DOTA)

$(DOTA): $(OBJS)
	$(AR) $(ARFLAGS) $(DOTA) `$(LORDER) $(OBJS) | $(TSORT)`

clean:
	rm -f *.o

clobber: clean
	rm -f $(DOTA)

install: all libxchoosemsgs.str
	$(INS) -f $(USRLIB) -m 0644 $(DOTA)
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 libxchoosemsgs.str
