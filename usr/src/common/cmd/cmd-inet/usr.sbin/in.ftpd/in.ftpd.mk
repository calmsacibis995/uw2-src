#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.ftpd/in.ftpd.mk	1.18.13.2"
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

LOCALDEF=	-DSYSV -DSTRNET -DBSD_COMP
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lresolv -liaf -lgen

OBJS=		ftpd.o ftpcmd.o getusershell.o glob.o popen.o logwtmp.o vers.o security.o

all:		in.ftpd

in.ftpd:	$(OBJS)
		$(CC) -o in.ftpd  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.ftpd

clean:
		rm -f $(OBJS) y.tab.c ftpcmd.c y.tab.h

clobber:	clean
		rm -f in.ftpd

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

ftpcmd.o:	ftpcmd.y \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/arpa/ftp.h \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/ctype.h \
		$(INC)/pwd.h \
		$(INC)/setjmp.h \
		$(INC)/syslog.h \
		$(INC)/arpa/telnet.h \
		$(FRC)

ftpd.o:		ftpd.c \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		../security.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/stat.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/sys/file.h \
		$(INC)/sys/wait.h \
		$(INC)/netinet/in.h \
		$(INC)/arpa/ftp.h \
		$(INC)/arpa/inet.h \
		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/pwd.h \
		$(INC)/setjmp.h \
		$(INC)/netdb.h \
		$(INC)/errno.h \
		$(INC)/syslog.h \
		$(INC)/varargs.h \
		$(INC)/fcntl.h \
		$(INC)/shadow.h \
		$(FRC)

getusershell.o:	getusershell.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/file.h \
		$(INC)/sys/stat.h \
		$(INC)/ctype.h \
		$(INC)/stdio.h \
		$(FRC)

glob.o:		../../usr.bin/ftp/glob.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/stat.h \
		$(INC)/dirent.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/pwd.h \
		$(FRC)
		$(CC) $(CFLAGS) $(DEFLIST) -c ../../usr.bin/ftp/glob.c

logwtmp.o:	logwtmp.c \
		$(INC)/sys/types.h \
		$(INC)/sys/file.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/utmp.h \
		$(INC)/fcntl.h \
		$(FRC)

popen.o:	popen.c \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../security.h \
		$(INC)/sys/types.h \
		$(INC)/signal.h \
		$(INC)/stdio.h \
		$(FRC)

vers.o:		vers.c \
		$(FRC)

security.o:	../security.c \
		$(INC)/sys/secsys.h \
		$(INC)/priv.h \
		$(INC)/sys/errno.h \
		../security.h \
		$(FRC)
		$(CC) $(CFLAGS) $(DEFLIST) -I.. -c ../security.c
