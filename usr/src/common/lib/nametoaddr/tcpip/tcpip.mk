#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/tcpip/tcpip.mk	1.2.10.1"
#ident	"$Header: $"

# Makefile for tcpip.so

include $(LIBRULES)

LIBSODIR=	/usr/lib/
LIBNAME=	tcpip.so
OBJECTS=	tcpip.o file_db.o tcpip_mt.o
SRCS=		$(OBJECTS:.o=.c)

LOCALDEF=	-D_NSL_RPC_ABI -DPIC -D_REENTRANT $(PICFLAG)
LOCALLDFLAGS=	-dy -G -ztext -h $(LIBSODIR)$(LIBNAME) 

all:		$(LIBNAME)

tcpip.so:	$(OBJECTS)
		$(LD) $(LOCALLDFLAGS) -o $(LIBNAME) $(OBJECTS) -l nsl

INCLUDES=	$(INC)/stdio.h $(INC)/ctype.h $(INC)/sys/types.h \
	   		$(INC)/sys/socket.h $(INC)/netinet/in.h \
			$(INC)/netdb.h $(INC)/xti.h $(INC)/netconfig.h \
			$(INC)/netdir.h $(INC)/string.h \
			$(INC)/sys/param.h $(INC)/sys/utsname.h tcpip_mt.h

tcpip.o:   	$(INC)/stdio.h $(INC)/ctype.h $(INC)/sys/types.h \
	   		$(INC)/sys/socket.h $(INC)/netinet/in.h \
			$(INC)/netdb.h $(INC)/tiuser.h $(INC)/netconfig.h \
			$(INC)/netdir.h $(INC)/string.h \
			$(INC)/sys/param.h $(INC)/sys/utsname.h \
			tcpip_mt.h tcpip.c

tcpip_mt.o:	$(INC)/mt.h tcpip_mt.h tcpip.c

file_db.o:	$(INC)/stdio.h $(INC)/ctype.h $(INC)/string.h  \
	   		$(INC)/sys/types.h $(INC)/sys/socket.h \
			$(INC)/netdb.h $(INC)/netinet/in.h file_db.c

clean:
		rm -f $(OBJECTS)

clobber:	clean
		rm -f $(LIBNAME)

install:	all
		$(INS) -f $(USRLIB) $(LIBNAME)
