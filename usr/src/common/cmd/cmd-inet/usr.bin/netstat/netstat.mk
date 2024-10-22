#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/netstat/netstat.mk	1.16.11.2"
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
# 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#       (c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#

#
#
#	Copyright 1987, 1988 Lachman Associates, Incorporated (LAI)
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

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP -D_KMEMUSER
INSDIR=		$(USRBIN)
OWN=		bin
GRP=		sys

LDLIBS=		-lsocket -lnsl $(LIBELF)

OBJS=		if.o inet.o main.o route.o unix.o

all:		netstat

netstat:	$(OBJS)
		$(CC) -o netstat  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 02555 -u $(OWN) -g $(GRP) netstat

clean:
		rm -f $(OBJS) core a.out

clobber:	clean
		rm -f netstat

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

if.o:		if.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		$(INC)/netinet/in_var.h \
		$(INC)/stdio.h \
		$(FRC)

inet.o:		inet.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/sys/socketvar.h \
		$(INC)/sys/protosw.h \
		$(INC)/netinet/in.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in_systm.h \
		$(INC)/netinet/in_pcb.h \
		$(INC)/netinet/ip.h \
		$(INC)/netinet/ip_icmp.h \
		$(INC)/netinet/icmp_var.h \
		$(INC)/netinet/ip_var.h \
		$(INC)/netinet/tcp.h \
		$(INC)/netinet/tcpip.h \
		$(INC)/netinet/tcp_seq.h \
		$(INC)/netinet/tcp_fsm.h \
		$(INC)/netinet/tcp_timer.h \
		$(INC)/netinet/tcp_var.h \
		$(INC)/netinet/tcp_debug.h \
		$(INC)/netinet/udp.h \
		$(INC)/netinet/udp_var.h \
		$(INC)/netdb.h \
		$(FRC)

main.o:		main.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/ctype.h \
		$(INC)/errno.h \
		$(INC)/netdb.h \
		$(INC)/nlist.h \
		$(INC)/stdio.h \
		$(INC)/fcntl.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/mod.h \
		$(FRC)

route.o:	route.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		$(INC)/netinet/in_var.h \
		$(INC)/net/route.h \
		$(INC)/netinet/ip_str.h \
		$(INC)/netdb.h \
		$(FRC)

unix.o:		unix.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/sys/sockmod.h \
		$(INC)/sys/socketvar.h \
		$(INC)/sys/un.h \
		$(FRC)
