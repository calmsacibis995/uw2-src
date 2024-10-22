#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.bin/talk/talk.mk	1.13.10.3"
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

#LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP
INSDIR=		$(USRBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lcurses -lsocket -lnsl

OBJS=		talk.o get_names.o display.o io.o ctl.o init_disp.o\
	  	msgs.o get_addrs.o ctl_transact.o invite.o look_up.o

all:		otalk

otalk:		$(OBJS)
		$(CC) -o otalk  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) otalk

clean:
		rm -f $(OBJS) core a.out

clobber:	clean
		rm -f otalk

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

ctl.o:		ctl.c \
		talk_ctl.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

ctl_transact.o:	ctl_transact.c \
		talk_ctl.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(INC)/sys/time.h \
		$(FRC)

display.o:	display.c \
		talk.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

get_addrs.o:	get_addrs.c \
		talk_ctl.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

get_names.o:	get_names.c \
		talk.h \
		ctl.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

init_disp.o:	init_disp.c \
		talk.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(INC)/signal.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/stropts.h \
		$(FRC)

invite.o:	invite.c \
		talk_ctl.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(INC)/sys/time.h \
		$(INC)/signal.h \
		$(INC)/setjmp.h \
		$(FRC)

io.o:		io.c \
		talk.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/sys/time.h \
		$(INC)/sys/filio.h \
		$(FRC)

look_up.o:	look_up.c \
		talk_ctl.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

msgs.o:		msgs.c \
		$(INC)/signal.h \
		$(INC)/stdio.h \
		$(INC)/sys/time.h \
		talk.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)

talk.o:		talk.c \
		talk.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/curses.h \
		$(INC)/utmp.h \
		$(INC)/errno.h \
		$(FRC)
