#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/telnet/telnet.mk	1.1.1.2"
#ident	"$Header: $"

#
#	STREAMware TCP
#	Copyright 1987, 1993 Lachman Technology, Inc.
#	All Rights Reserved.
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
INSDIR=		$(USRBIN)
OWN=		bin
GRP=		bin

LOCALDEF=	-DSYSV -DSTRNET -DKLUDGELINEMODE \
		-DHAS_IP_TOS -DNEED_GETTOS -DSRCRT \
		-DUSE_TERMIO

LDLIBS=		-L../../usr.lib/libtelnet -lcurses -lsocket -lnsl -ltelnet -lresolv

OBJS=		commands.o main.o network.o ring.o sys_bsd.o telnet.o \
		terminal.o tn3270.o utilities.o

all:		telnet

telnet:		$(OBJS)
		$(CC) -o telnet  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) telnet

clean:
		rm -f $(OBJS) core a.out

clobber:	clean
		rm -f telnet

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

commands.o:	$(INC)/sys/types.h \
		$(INC)/sys/file.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/signal.h \
		$(INC)/netdb.h \
		$(INC)/ctype.h \
		$(INC)/varargs.h \
		$(INC)/arpa/telnet.h \
		$(INC)/netinet/in_systm.h \
		$(INC)/netinet/ip.h \
		$(INC)/arpa/inet.h \
		general.h \
		ring.h \
		externs.h \
		defines.h \
		types.h \
		$(FRC)

main.o:		main.c \
		$(INC)/sys/types.h \
		ring.h \
		externs.h \
		defines.h \
		$(FRC)

network.o:	network.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/time.h \
		$(INC)/errno.h \
		$(INC)/arpa/telnet.h \
		ring.h \
		defines.h \
		externs.h \
		fdset.h \
		$(FRC)

ring.o:		ring.c \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		ring.h \
		general.h \
		$(FRC)

sys_bsd.o:	sys_bsd.c \
		$(INC)/fcntl.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/filio.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/arpa/telnet.h \
		ring.h \
		fdset.h \
		defines.h \
		externs.h \
		types.h \
		$(FRC)

telnet.o:	telnet.c \
		$(INC)/signal.h \
		$(INC)/arpa/telnet.h \
		$(INC)/string.h \
		$(INC)/ctype.h \
		ring.h \
		defines.h \
		types.h \
		general.h \
		$(FRC)

terminal.o:	terminal.c \
		$(INC)/arpa/telnet.h \
		$(INC)/sys/types.h \
		ring.h \
		externs.h \
		types.h \
		$(FRC)

tn3270.o:	tn3270.c \
		$(INC)/sys/types.h \
		$(INC)/arpa/telnet.h \
		general.h \
		defines.h \
		ring.h \
		externs.h \
		fdset.h \
		$(FRC)

utilities.o:	utilities.c \
		$(INC)/arpa/telnet.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h \
		$(INC)/ctype.h \
		general.h \
		fdset.h \
		ring.h \
		defines.h \
		externs.h \
		$(FRC)
