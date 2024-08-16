#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/tcpip_nis/tcpip_nis.mk	1.1"
#ident  "$Header: $"

#	Makefile for nis.so

include $(LIBRULES)

LIBNAME=	tcpip_nis.so
OBJECTS=	tcpip.o tcpip_nis.o ckypbind.o tcpip_nis_mt.o

LOCALDEF=-D_NSL_RPC_ABI -DPIC -D_REENTRANT $(PICFLAG) -DYP
LOCALLDFLAGS= -dy -G -ztext -h /usr/lib/$(LIBNAME)


all:		$(LIBNAME)

tcpip_nis.so:	$(OBJECTS)
		$(LD) $(LOCALLDFLAGS) -o $(LIBNAME) $(OBJECTS) -lnsl

tcpip.o:   	../tcpip/tcpip.c
			$(CC) -c $(DEFLIST) -I../tcpip_nis ../tcpip/tcpip.c

tcpip_nis.o: tcpip_nis.c \
	$(INC)/stdio.h $(INC)/rpc/rpc.h $(INC)/sys/socket.h \
	$(INC)/netinet/in.h $(INC)/netdb.h $(INC)/string.h  \
	$(INC)/rpcsvc/yp_prot.h $(INC)/rpcsvc/ypclnt.h tcpip_nis_mt.h

ckypbind.o: ckypbind.c \
	$(INC)/stdio.h $(INC)/sys/types.h $(INC)/rpc/rpc.h \
	$(INC)/rpc/rpc_com.h $(INC)/rpc/rpcb_prot.h \
	$(INC)/rpcsvc/yp_prot.h $(INC)/netinet/in.h $(INC)/sys/socket.h \
	$(INC)/netconfig.h

tcpip_nis_mt.o: tcpip_nis_mt.c \
	$(INC)/mt.h tcpip_nis_mt.h tcpip_nis.c

clean:
		rm -f $(OBJECTS)

clobber:
		rm -f $(OBJECTS) $(LIBNAME)

install:	all
		$(INS) -f $(USRLIB) $(LIBNAME)

size:		all
		$(SIZE) $(LIBNAME)

strip:		all
		$(STRIP) $(LIBNAME)

lintit:
