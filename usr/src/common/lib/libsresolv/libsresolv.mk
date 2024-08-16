#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libresolv:common/lib/libsresolv/libsresolv.mk	1.1.1.10"
#ident	"$Header: $"

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

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

MORECPP=	-DDEBUG

LIBSODIR=	/usr/lib/
LIBNAME=	libresolv.so.1
OUTNAME=	libresolv.so
RS_LDFLAGS=	-G -dy -h $(LIBSODIR)$(LIBNAME)
#LOCALDEF=	-DDEBUG -D_RESOLV_ABI -D_REENTRANT $(PICFLAG)
LOCALDEF=	-D_RESOLV_ABI -D_REENTRANT $(PICFLAG)

OBJS=		gthostnamadr.o res_comp.o res_debug.o res_init.o \
		res_mkquery.o res_query.o res_send.o sethostent.o \
		strcasecmp.o libres_mt.o gtservnamprt.o

all:		$(OBJS)
		$(LD) $(RS_LDFLAGS) -o $(OUTNAME) $(OBJS) -l socket -l nsl

install:	all
		if [ x$(CCSTYPE) != xCOFF ] ; \
		then \
		$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) $(OUTNAME) ; \
		rm -f $(USRLIB)/$(LIBNAME) ; \
		ln $(USRLIB)/$(OUTNAME) $(USRLIB)/$(LIBNAME) ; \
		fi

clean:
		rm -f *.o

clobber:	clean
		rm -f *.so

lintint:

#
# Header dependencies
#
gthostnamadr.o:	gthostnamadr.c \
		$(INC)/sys/byteorder.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/arpa/inet.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

gtservnamprt.o:	gtservnamprt.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netdb.h \
		$(INC)/ctype.h \
		$(INC)/stdlib.h \
		$(INC)/sys/byteorder.h \
		res.h \
		libres_mt.h \
		$(FRC)

res_comp.o:	res_comp.c \
		$(INC)/sys/types.h \
		$(INC)/stdio.h \
		$(INC)/arpa/nameser.h \
		res.h \
		$(FRC)

res_debug.o:	res_debug.c \
		$(INC)/sys/byteorder.h \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/arpa/nameser.h \
		res.h \
		$(FRC)

res_init.o:	res_init.c \
		$(INC)/sys/byteorder.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

res_mkquery.o:	res_mkquery.c \
		$(INC)/sys/byteorder.h \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

res_query.o:	res_query.c \
		$(INC)/sys/byteorder.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

res_send.o:	res_send.c \
		$(INC)/sys/byteorder.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/uio.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/arpa/nameser.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

sethostent.o:	sethostent.c \
		$(INC)/sys/types.h \
		$(INC)/arpa/nameser.h \
		$(INC)/netinet/in.h \
		$(INC)/resolv.h \
		res.h \
		$(FRC)

strcasecmp.o:	strcasecmp.c \
		res.h \
		$(FRC)
