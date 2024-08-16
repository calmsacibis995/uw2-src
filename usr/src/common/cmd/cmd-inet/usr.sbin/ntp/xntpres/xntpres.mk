#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/xntpres/xntpres.mk	1.1.1.2"
#ident	"$Header: $"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
#

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
#       (c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  
#

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DSTREAM -DREFCLOCK -DNO_SIGNED_CHAR_DECL
LOCALINC=	-I../include

INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen ../lib/libntp.a

OBJS=		xntpres.o version.o

all:		xntpres

xntpres:	$(OBJS)
		$(CC) -o xntpres  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) xntpres

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f xntpres

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

xntpres.o:	$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/fcntl.h \
		$(INC)/sys/fcntl.h \
		$(INC)/netdb.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_request.h \
		$(FRC)
