#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.telnetd/in.telnetd.mk	1.1.2.2"
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

LOCALDEF=       -DSYSV -DSTRNET -DKLUDGELINEMODE \
		-DHAS_IP_TOS -DNEED_GETTOS -DSRCRT \
		-DUSE_TERMIO -DDIAGNOSTICS

INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-L../../usr.lib/libtelnet -lcurses -lsocket -ltelnet -liaf -lnsl -lresolv -lgen

OBJS=		telnetd.o state.o termstat.o slc.o sys_term.o \
		utility.o global.o authenc.o

all:		in.telnetd

in.telnetd:	$(OBJS)
		$(CC) -o in.telnetd  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.telnetd

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f in.telnetd

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

telnetd.o:	telnetd.c \
		telnetd.h \
		defs.h \
		ext.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/termio.h \
		$(INC)/fcntl.h \
		$(INC)/arpa/telnet.h \
		$(FRC)

global.o:	global.c \
		defs.h \
		ext.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/termio.h \
		$(INC)/fcntl.h \
		$(INC)/arpa/telnet.h \
		$(FRC)

slc.o:		slc.c \
		telnetd.h \
		defs.h \
		ext.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/termio.h \
		$(INC)/fcntl.h \
		$(INC)/arpa/telnet.h \
		$(FRC)

state.o:	state.c \
		telnetd.h \
		defs.h \
		ext.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/termio.h \
		$(INC)/fcntl.h \
		$(INC)/arpa/telnet.h \
		$(FRC)

sys_term.o:	sys_term.c \
		telnetd.h \
		defs.h \
		ext.h \
		pathnames.h \
		$(INC)/utmp.h \
		$(INC)/sys/stream.h \
		$(INC)/sys/tty.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/termio.h \
		$(INC)/fcntl.h \
		$(INC)/arpa/telnet.h \
		$(FRC)

termstat.o:	termstat.c \
		telnetd.h \
		defs.h \
		ext.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/termio.h \
		$(INC)/fcntl.h \
		$(INC)/arpa/telnet.h \
		$(FRC)

utility.o:	utility.c \
		telnetd.h \
		defs.h \
		ext.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/string.h \
		$(INC)/termio.h \
		$(INC)/fcntl.h \
		$(INC)/arpa/telnet.h \
		$(FRC)
