#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/in.xntpd.mk	1.1.1.2"
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

CLOCKDEFS=	-DLOCAL_CLOCK
LOCALDEF=	-DSYSV -DSVR42 -DSTRNET -DSTREAM -DREFCLOCK \
		-DNO_SIGNED_CHAR_DECL $(CLOCKDEFS) \
		-DCONFIG_FILE=\"/etc/inet/ntp.conf\"
LOCALINC=	-I../include

INSDIR=		$(USRSBIN)
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lgen ../lib/libntp.a

OBJS=		ntp_config.o ntp_control.o ntp_io.o ntp_leap.o \
		ntp_lf.o ntp_monitor.o ntp_peer.o ntp_proto.o \
		ntp_refclock.o ntp_request.o ntp_restrict.o ntp_timer.o \
		ntp_unixclk.o ntp_util.o ntpd.o refclk_chu.o \
		refclk_conf.o refclk_local.o refclk_pst.o \
		refclk_wwvb.o version.o

all:		in.xntpd

in.xntpd:		$(OBJS)
		$(CC) -o in.xntpd  $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.xntpd

clean:
		rm -f $(OBJS)

clobber:	clean
		rm -f in.xntpd

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:

#
# Header dependencies
#

ntp_config.o:	$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/wait.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_refclock.h \
		$(FRC)

ntp_control.o:	$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/file.h \
		$(INC)/sys/time.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_refclock.h \
		../include/ntp_control.h \
		$(FRC)

ntp_io.o:	$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/param.h \
		$(INC)/sys/fs/s5param.h \
		$(INC)/sys/ioctl.h \
		$(INC)/fcntl.h \
		$(INC)/sys/fcntl.h \
		$(INC)/sys/time.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_refclock.h \
		$(FRC)

ntp_leap.o:	$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		$(FRC)

ntp_lf.o:	$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		$(FRC)

ntp_monitor.o:	$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/file.h \
		$(INC)/sys/time.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		$(FRC)

ntp_peer.o:	$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		$(FRC)

ntp_proto.o:	$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		$(FRC)

ntp_refclock.o:	$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_refclock.h \
		$(FRC)

ntp_request.o:	$(INC)/stdio.h \
		$(INC)/string.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/file.h \
		$(INC)/sys/time.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_request.h \
		../include/ntp_control.h \
		../include/ntp_refclock.h \
		$(FRC)

ntp_restrict.o:	$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		$(FRC)

ntp_timer.o:	$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		$(FRC)

ntp_unixclk.o:	$(INC)/stdio.h \
		$(INC)/sys/fcntl.h \
		$(INC)/nlist.h \
		$(INC)/sys/types.h \
		$(INC)/sys/time.h \
		$(INC)/sys/file.h \
		$(INC)/sys/stat.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp_unixtime.h \
		$(FRC)

ntp_util.o:	$(INC)/stdio.h \
		$(INC)/sys/fcntl.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/string.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/file.h \
		$(INC)/sys/time.h \
		$(INC)/net/if.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		$(FRC)

ntpd.o:		$(INC)/stdio.h \
		$(INC)/signal.h \
		$(INC)/sys/signal.h \
		$(INC)/errno.h \
		$(INC)/sys/errno.h \
		$(INC)/sys/types.h \
		$(INC)/sys/signal.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sys/socket.h \
		$(INC)/sys/time.h \
		$(INC)/sys/resource.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		$(FRC)

refclk_chu.o:	$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/sys/time.h \
		$(INC)/sys/file.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sgtty.h \
		$(INC)/stropts.h \
		$(INC)/sys/stropts.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_refclock.h \
		../include/ntp_unixtime.h \
		$(FRC)

refclk_conf.o:	$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_refclock.h \
		$(FRC)

refclk_local.o:	$(INC)/stdio.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/sys/time.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_refclock.h \
		$(FRC)

refclk_pst.o:	$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/sys/time.h \
		$(INC)/sys/file.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sgtty.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_refclock.h \
		../include/ntp_unixtime.h \
		$(FRC)

refclk_wwvb.o:	$(INC)/stdio.h \
		$(INC)/ctype.h \
		$(INC)/sys/types.h \
		$(INC)/sys/socket.h \
		$(INC)/netinet/in.h \
		$(INC)/sys/time.h \
		$(INC)/sys/file.h \
		$(INC)/sys/ioctl.h \
		$(INC)/sgtty.h \
		../include/ntp_syslog.h \
		$(INC)/syslog.h \
		$(INC)/sys/syslog.h \
		../include/ntp_fp.h \
		../include/ntp.h \
		../include/ntp_refclock.h \
		../include/ntp_unixtime.h \
		$(FRC)

