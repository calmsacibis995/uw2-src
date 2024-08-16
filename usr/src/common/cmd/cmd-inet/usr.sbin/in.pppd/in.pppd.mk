#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.pppd/in.pppd.mk	1.1.1.3"
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
LIBDIR=		$(USRLIB)/ppp
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen

DOBJS=		in.pppd.o sock_supt.o  gtphostent.o gtphstnamadr.o

POBJS=		ppp.o sock_supt.o

TOBJS=		pppstat.o sock_supt.o

all:		in.pppd ppp pppstat pppconf

in.pppd:	$(DOBJS)
		$(CC) -o in.pppd  $(LDFLAGS) $(DOBJS) $(LDLIBS) $(SHLIBS)

ppp:		$(POBJS)
		$(CC) -o ppp  $(LDFLAGS) $(POBJS) $(LDLIBS) $(SHLIBS)

pppstat:	$(TOBJS)
		$(CC) -o pppstat  $(LDFLAGS) $(TOBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.pppd
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) pppstat
		if [ ! -d $(LIBDIR) ] ; then mkdir  $(LIBDIR) 2>/dev/null ; fi
		$(INS) -f $(LIBDIR) -m 04711 -u root -g $(GRP) ppp
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) pppconf

clean:
		rm -f $(DOBJS) $(POBJS) $(TOBJS) 

clobber:	clean
		rm -f in.pppd in.pppstat in.ppp ppp pppstat pppconf

FRC:

#
# Header dependencies
#

gtphostent.o:	gtphostent.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/string.h \
		$(INC)/netdb.h \
		$(INC)/ctype.h \
		$(INC)/sys/syslog.h \
		$(INC)/sys/conf.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		$(INC)/netinet/in_var.h \
		$(INC)/netinet/ppp.h \
		$(INC)/netinet/pppcnf.h \
		$(FRC)

gtphstnamadr.o:	gtphstnamadr.c \
		$(INC)/stdio.h \
		$(INC)/netdb.h \
		$(INC)/sys/types.h \
		$(INC)/sys/file.h \
		$(INC)/ctype.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/conf.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		$(INC)/netinet/in_var.h \
		$(INC)/netinet/ppp.h \
		$(INC)/netinet/pppcnf.h \
		$(FRC)	

ppp.o: 		ppp.c \
		pppd.h \
		$(INC)/stdio.h \
		$(INC)/pwd.h \
		$(INC)/fcntl.h \
		$(INC)/string.h \
		$(INC)/sys/stropts.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/syslog.h \
		$(INC)/netinet/in.h \
		$(FRC)	

pppstat.o: 	pppstat.c \
		pppd.h \
		$(INC)/stdio.h \
		$(INC)/pwd.h \
		$(INC)/fcntl.h \
		$(INC)/string.h \
		$(INC)/sys/stropts.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/syslog.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		$(INC)/netinet/in_var.h \
		$(INC)/netinet/ppp.h \
		$(FRC)

in.pppd.o:	in.pppd.c \
		pppd.h \
		$(INC)/dial.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/errno.h \
		$(INC)/unistd.h \
		$(INC)/grp.h \
		$(INC)/pwd.h \
		$(INC)/signal.h \
		$(INC)/varargs.h \
		$(INC)/sys/stream.h \
		$(INC)/sys/stropts.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/poll.h \
		$(INC)/string.h \
		$(INC)/netdb.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		$(INC)/netinet/in_var.h \
		$(INC)/netinet/ppp.h \
		$(INC)/netinet/pppcnf.h \
		$(INC)/time.h \
		$(INC)/sys/select.h \
		$(INC)/sys/stat.h \
		$(FRC)

