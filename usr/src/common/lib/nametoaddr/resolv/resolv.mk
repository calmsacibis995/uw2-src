#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/resolv/resolv.mk	1.3.10.6"
#ident	"$Header: $"

#	Makefile for resolv.so

include $(LIBRULES)

LIBSODIR=	/usr/lib/
LIBNAME=	resolv.so
LOCALDEF=	-D_RESOLV_ABI -D_REENTRANT $(PICFLAG)
LOCAL_LDFLAGS=	-dy -G -ztext -h $(LIBSODIR)$(LIBNAME)

OBJS=		resolv.o resolv_mt.o

all:		$(OBJS)
		$(LD) $(LOCAL_LDFLAGS) -o $(LIBNAME) $(OBJS) \
			 -l resolv -l socket -l nsl

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f $(LIBNAME)

install:	all
		$(INS) -f $(USRLIB) $(LIBNAME)

size:		all
		$(SIZE) $(LIBNAME)

strip:		all
		$(STRIP) $(LIBNAME)

lintit:

#
# header dependencies
#
resolv.o:	resolv.c \
		resolv_mt.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/xti.h \
		$(INC)/netconfig.h \
		$(INC)/netdir.h \
		$(INC)/string.h \
		$(INC)/fcntl.h \
		$(INC)/sys/param.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/utsname.h \
		$(INC)/net/if.h \
		$(INC)/stropts.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/syslog.h \
		$(FRC)

resolv_mt.o:	resolv_mt.c \
		resolv_mt.h \
		$(INC)/stdlib.h \
		$(FRC)
