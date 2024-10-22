#ident	"@(#)ucb:common/ucblib/librpcsoc/librpcsoc.mk	1.4"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#


#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#
# Makefile for librpcsoc.a
#

include $(LIBRULES)

INSDIR	= $(ROOT)/$(MACH)/usr/ucblib
LIBNAME	= librpcsoc.a
INCFLAGS = -I$(INC) -DPORTMAP
ARFLAGS = cr
OWN = bin
GRP = bin
CPPFLAGS = -O $(PICFLAG) $(INCFLAGS)
LOCAL_CFLAGS	= $(CFLAGS) $(CPPFLAGS) 

OBJS =	clnt_udp.o clnt_tcp.o svc_udp.o svc_tcp.o rtime.o


SRCS = $(OBJS:.o=.c)

.c.o:
	$(CC) $(LOCAL_CFLAGS) $(DEFLIST) -c $<

$(LIBNAME): $(OBJS)
	-rm -f $(LIBNAME);
	$(AR) $(ARFLAGS) $(LIBNAME) `$(LORDER) $(OBJS) | $(TSORT)`

lib: $(LIBNAME)

all:	lib

 
install: lib
	$(INS) -m 644 -u $(OWN) -g $(GRP) -f $(INSDIR) $(LIBNAME)
lint:
	$(LINT) $(INCFLAGS) $(SRCS)

clean:
	rm -f $(OBJS)

clobber: clean
	rm -f $(LIBNAME)

