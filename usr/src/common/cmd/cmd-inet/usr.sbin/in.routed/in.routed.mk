#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.routed/in.routed.mk	1.13.11.2"
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
#       (c) 1990,1991  UNIX System Laboratories, Inc.
# 	          All rights reserved.
#  
#

include $(CMDRULES)

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP -D_KMEMUSER
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen

OBJS=		af.o if.o inet.o input.o main.o output.o \
		startup.o tables.o timer.o trace.o

all:		in.routed

in.routed:	$(OBJS)
		$(CC) -o in.routed $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.routed

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f in.routed

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

af.o:		af.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(FRC)

if.o:		if.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(FRC)

inet.o:		inet.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(FRC)

input.o:	input.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(INC)/syslog.h \
		$(FRC)

main.o:		main.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stropts.h \
		$(INC)/net/if.h \
		$(INC)/errno.h \
		$(INC)/signal.h \
		$(INC)/syslog.h \
		$(INC)/fcntl.h \
		$(FRC)

output.o:	output.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(INC)/net/if.h \
		$(FRC)

startup.o:	startup.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(INC)/sys/sockio.h \
		$(INC)/net/if.h \
		$(INC)/syslog.h \
		$(FRC)

tables.o:	tables.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(INC)/sys/sockio.h \
		$(INC)/sys/ioctl.h \
		$(INC)/errno.h \
		$(INC)/syslog.h \
		$(FRC)

timer.o:	timer.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(FRC)

trace.o:	trace.c \
		defs.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/stream.h \
		$(INC)/net/route.h \
		$(INC)/netinet/in.h \
		$(INC)/protocols/routed.h \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		trace.h \
		interface.h \
		table.h \
		af.h \
		$(INC)/sys/stat.h \
		$(FRC)
