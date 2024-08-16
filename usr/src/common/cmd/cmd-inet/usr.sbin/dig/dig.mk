#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/dig/dig.mk	1.2"
#ident	"$Header: $"

#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#

#
#	System V STREAMS TCP - Release 4.0
#
#   Copyright 1990 Interactive Systems Corporation,(ISC)
#   All Rights Reserved.
#
#	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
#	All Rights Reserved.
#
#	The copyright above and this notice must be preserved in all
#	copies of this source code.  The copyright above does not
#	evidence any actual or intended publication of this source
#	code.
#
#	This is unpublished proprietary trade secret source code of
#	Lachman Associates.  This source code may not be copied,
#	disclosed, distributed, demonstrated or licensed except as
#	expressly authorized by Lachman Associates.
#
#	System V STREAMS TCP was jointly developed by Lachman
#	Associates and Convergent Technologies.
#
# Copyright (c) 1988 Regents of the University of California.
# All rights reserved.
#
# Redistribution and use in source and binary forms are permitted
# provided that this notice is preserved and that due credit is given
# to the University of California at Berkeley. The name of the University
# may not be used to endorse or promote products derived from this
# software without specific prior written permission. This software
# is provided ``as is'' without express or implied warranty.
#
####
#### Updated 3/27/89 for 'dig' version 1.0 at University of Southern
#### California Information Sciences Institute (USC-ISI).
####
#### Modified & distributed with 'dig' version 2.0 from USC-ISI (9/1/90).
####
####

include $(CMDRULES)

LOCALDEF =	-DSYSV -DDEBUG
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lresolv

OBJS =		herror.o res_comp.o res_debug.o res_init.o res_mkquery.o \
		res_query.o res_send.o strcasecmp.o gethtnmadr.o \
		sethostent.o qtime.o 

all:		dig

dig:		libresolv.a dig.o list.o
		$(CC) -o dig  $(CFLAGS) $(LDFLAGS) dig.o list.o \
			-L`pwd` $(LDLIBS) $(SHLIBS)

libresolv.a:	$(OBJS)
		$(AR) cru libresolv.a $(OBJS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) dig

clean:
		rm -f dig.o $(OBJS) list.o a.out core errs

clobber:	clean
		rm -f dig dig.g dig.local libresolv.a

FRC:

#
# Header dependencies
#
herror.o:	herror.c \
		$(INC)/sys/types.h \
		$(FRC)

res_comp.o:	res_comp.c \
		hfiles.h \
		nameser.h \
		$(INC)/sys/types.h \
		$(INC)/stdio.h \
		$(FRC)

res_debug.o:	res_debug.c \
		hfiles.h \
		nameser.h \
		resolv.h \
		pflag.h \
		$(INC)/sys/types.h \
		$(INC)/stdio.h \
		$(INC)/netinet/in.h \
		$(FRC)

res_init.o:	res_init.c \
		hfiles.h \
		nameser.h \
		resolv.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/stdio.h \
		$(INC)/netinet/in.h \
		$(FRC)

res_mkquery.o:	res_mkquery.c \
		hfiles.h \
		nameser.h \
		resolv.h \
		$(INC)/sys/types.h \
		$(INC)/stdio.h \
		$(INC)/netinet/in.h \
		$(FRC)

res_query.o:	res_query.c \
		hfiles.h \
		param.h \
		nameser.h \
		resolv.h \
		netdb.h \
		$(INC)/sys/types.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/socket.h \
		$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		$(FRC)

res_send.o:	res_send.c \
		hfiles.h \
		param.h \
		nameser.h \
		resolv.h \
		netdb.h \
		qtime.h \
		pflag.h \
		$(INC)/sys/types.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/sys/time.h \
		$(INC)/time.h \
		$(FRC)

strcasecmp.o:	strcasecmp.c \
		$(FRC)

gethtnmadr.o:	gethtnmadr.c \
		hfiles.h \
		param.h \
		$(INC)/signal.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/ctype.h \
		netdb.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/arpa/inet.h \
		nameser.h \
		$(FRC)

sethostent.o:	sethostent.c \
		hfiles.h \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		resolv.h \
		nameser.h \
		$(FRC)

qtime.o:	qtime.c \
		qtime.h \
		$(INC)/stdio.h \
		$(INC)/sys/time.h \
		$(INC)/time.h \
		$(INC)/sys/types.h \
		$(FRC)

dig.o:		dig.c \
		hfiles.h \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		resolv.h \
		$(INC)/sys/file.h \
		$(INC)/sys/fcntl.h \
		$(INC)/sys/stat.h \
		$(INC)/ctype.h \
		netdb.h \
		nameser.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/socket.h \
		$(INC)/setjmp.h \
		$(INC)/sys/time.h \
		$(INC)/time.h \
		$(FRC)

list.o:		list.c \
		hfiles.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		netdb.h \
		$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/ctype.h \
		nameser.h \
		resolv.h \
		res.h \
		$(FRC)
