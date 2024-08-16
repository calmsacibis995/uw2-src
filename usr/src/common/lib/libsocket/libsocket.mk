#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libsocket:common/lib/libsocket/libsocket.mk	1.16.16.10"
#ident	"$Header: $"

#
# +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
# 		PROPRIETARY NOTICE (Combined)
# 
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
# 
# 
# 
# 		Copyright Notice 
# 
# Notice of copyright on this source code product does not indicate 
# publication.
# 
# 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
# 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
#	(c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  

include $(LIBRULES)
include lsock.rules

LIBSODIR=	/usr/lib/
LIBNAME=	libsocket.so.1
OUTNAME=	libsocket.so

LDFLAGS=	-G -dy -ztext -h $(LIBSODIR)$(LIBNAME)

OWN=		root
GRP=		bin

INETOBJS=	bindresvport.o byteorder.o ether_addr.o getnetbyaddr.o \
		getnetbyname.o getnetent.o getproto.o getprotoent.o \
		getprotoname.o getservent.o gtservbyname.o gtservbyport.o \
		inet_addr.o inet_lnaof.o inet_mkaddr.o inet_netof.o \
		inet_network.o rcmd.o rexec.o ruserpass.o inet_sethost.o \
		nd_gethost.o herror.o ifignore.o innetgr.o getnetgrent.o

SOCKOBJS=	accept.o bind.o connect.o socket.o socketpair.o \
		shutdown.o getsockopt.o setsockopt.o listen.o \
		receive.o send.o _conn_util.o _utility.o \
		getsocknm.o getpeernm.o setsocknm.o setpeernm.o s_ioctl.o \
		libsock_mt.o

LIBOBJS=	$(INETOBJS) $(SOCKOBJS)

all:
		@for i in inet socket;\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk all";\
			$(MAKE) -f $$i.mk all $(MAKEARGS) LS_DEF='$(LS_DEF)'; \
			cd ..;\
		done;\
		wait
		rm -f $(OUTNAME)
		if [ x$(CCSTYPE) != xCOFF ] ; \
		then \
		$(LD) $(LDFLAGS) -o $(OUTNAME) $(LIBOBJS) -l nsl ;\
		fi
		-rm -f $(LIBOBJS)

install:	all
		if [ x$(CCSTYPE) != xCOFF ] ; \
		then \
		$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) $(OUTNAME) ;\
		rm -f $(USRLIB)/$(LIBNAME) ; \
		ln $(USRLIB)/$(OUTNAME) $(USRLIB)/$(LIBNAME) ; \
		fi

clean: 		
		@for i in inet socket;\
		do\
			cd $$i;\
			/bin/echo "\n===== $(MAKE) -f $$i.mk clean";\
			$(MAKE) -f $$i.mk clean $(MAKEARGS);\
			cd ..;\
		done;\
		wait
		-rm -f $(LIBOBJS)

clobber:	clean
		-rm -f $(OUTNAME)
