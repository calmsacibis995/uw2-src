#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.named/in.named.mk	1.18.12.2"
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

LOCALDEF=	-DSYSV -DSTRNET -DDEBUG -DBSD=43
INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lresolv -lsocket -lnsl -lgen

OBJS=		db_dump.o db_load.o db_lookup.o db_reload.o db_save.o \
		db_update.o db_glue.o ns_forw.o ns_init.o ns_main.o ns_maint.o \
		ns_req.o ns_resp.o ns_sort.o ns_stats.o version.o
XOBJS=		xfer.o db_glue.o

all:		in.named named-xfer tooldir

in.named:	$(OBJS)
		$(CC) -o in.named $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

named-xfer:	$(XOBJS)
		$(CC) -o named-xfer $(LDFLAGS) $(XOBJS) $(LDLIBS) $(SHLIBS)

tooldir:
		@cd tools;\
		/bin/echo "\n===== $(MAKE) -f tools.mk all";\
		$(MAKE) -f tools.mk all $(MAKEARGS)

install:	in.named named-xfer
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.named
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) named-xfer
		@cd tools;\
		/bin/echo "\n===== $(MAKE) -f tools.mk install";\
		$(MAKE) -f tools.mk install $(MAKEARGS)

clobber: clean
		rm -f in.named named-xfer
		@cd tools;\
		/bin/echo "\n===== $(MAKE) -f tools.mk clobber";\
		$(MAKE) -f tools.mk clobber $(MAKEARGS)

clean:
		rm -f *.o
		@cd tools;\
		/bin/echo "\n===== $(MAKE) -f tools.mk clean";\
		$(MAKE) -f tools.mk clean $(MAKEARGS)

lintit:
		@cd tools;\
		/bin/echo "\n===== $(MAKE) -f tools.mk lintit";\
		$(MAKE) -f tools.mk lintit $(MAKEARGS)

FRC:

#
# Header dependencies
#

db_dump.o:	db_dump.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/netinet/in.h \
		$(INC)/netdb.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/arpa/nameser.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

db_load.o:	db_load.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/stat.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/ctype.h \
		$(INC)/netdb.h \
		$(INC)/arpa/nameser.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

db_lookup.o:	db_lookup.c \
		$(INC)/sys/types.h \
		$(INC)/stdio.h \
		$(INC)/arpa/nameser.h \
		db.h \
		$(FRC)

db_reload.o:	db_reload.c \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/arpa/nameser.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

db_save.o:	db_save.c \
		$(INC)/sys/types.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/arpa/nameser.h \
		db.h \
		$(FRC)

db_update.o:	db_update.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/time.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/arpa/nameser.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

gtdtablesize.o:	gtdtablesize.c \
		$(INC)/sys/resource.h \
		$(FRC)

ns_forw.o:	ns_forw.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/syslog.h \
		$(INC)/arpa/nameser.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

ns_init.o:	ns_init.c \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/time.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/errno.h \
		$(INC)/signal.h \
		$(INC)/syslog.h \
		$(INC)/ctype.h \
		$(INC)/arpa/nameser.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

ns_main.o:ns_main.c \
		$(INC)/sys/param.h \
		$(INC)/fcntl.h \
		$(INC)/sys/file.h \
		$(INC)/sys/time.h \
		$(INC)/sys/wait.h \
		$(INC)/sys/resource.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/sockio.h \
		$(INC)/netinet/in.h \
		$(INC)/net/if.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/errno.h \
		$(INC)/signal.h \
		$(INC)/netdb.h \
		$(INC)/arpa/nameser.h \
		$(INC)/arpa/inet.h \
		$(INC)/resolv.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

ns_maint.o:	ns_maint.c \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/time.h \
		$(INC)/unistd.h \
		$(INC)/utime.h \
		$(INC)/netinet/in.h \
		$(INC)/stdio.h \
		$(INC)/syslog.h \
		$(INC)/signal.h \
		$(INC)/errno.h \
		$(INC)/arpa/nameser.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

ns_req.o:		ns_req.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/uio.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/syslog.h \
		$(INC)/sys/file.h \
		$(INC)/arpa/nameser.h \
		$(INC)/fcntl.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

ns_resp.o:	ns_resp.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/param.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/file.h \
		$(INC)/netinet/in.h \
		$(INC)/syslog.h \
		$(INC)/arpa/nameser.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

ns_sort.o:	ns_sort.c \
		$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/file.h \
		$(INC)/netinet/in.h \
		$(INC)/syslog.h \
		$(INC)/arpa/nameser.h \
		ns.h \
		$(INC)/string.h \
		$(INC)/arpa/inet.h \
		db.h \
		$(FRC)

version.o:	version.c \
		$(FRC)
