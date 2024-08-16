#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.gated/in.gated.mk	1.1.2.5"
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
CONFDIR=	$(ETC)/inet
OWN=		bin
GRP=		bin

LDLIBS=		-lsocket -lnsl -lresolv -lgen

SIGNAL_H=	${INC}/sys/signal.h
YFLAGS =	-d
LFLAGS = 	-v

#does not run with nawk, be certain to use oawk
AWK=		/usr/bin/oawk
HEAD=		/usr/bin/head -50

OBJS= bgp.o bgp_init.o bgp_rt.o\
		egp.o egp_init.o egp_rt.o\
		ext.o hello.o\
		if.o if_rt.o\
		icmp.o inet.o\
		krt.o main.o rip.o\
		rt_control.o\
		rt_table.o\
		str.o task.o trace.o\
		parser.o lexer.o parse.o recvmsg.o \
		version.o

SRCS= bgp.c bgp_init.c bgp_rt.c\
		egp.c egp_init.c egp_rt.c\
		ext.c hello.c\
		if.c if_rt.c\
		icmp.c inet.c\
		krt.c main.c rip.c\
		rt_control.c\
		rt_table.c\
		str.c task.c trace.c\
		parser.c lexer.c parse.c recvmsg.c

INCLUDES= bgp.h defs.h egp.h egp_param.h hello.h if.h include.h\
		rip.h routed.h rt_control.h rt_table.h\
		snmp.h task.h trace.h\
		task_sig.h\
		parse.h parser.h

all:		in.gated ripquery

in.gated:	$(OBJS)
		$(CC) -o in.gated $(LDFLAGS) $(OBJS) $(LDLIBS) $(SHLIBS)

ripquery:	ripquery.o
		$(CC) -o ripquery $(LDFLAGS) ripquery.o $(LDLIBS) $(SHLIBS)

#
#  task.c requires a header built from the names of the signals
#  available on this system.
#

task.o:		task_sig.h

task_sig.h:	sigconv.awk ${SIGNAL_H}
		${AWK} -f sigconv.awk < ${SIGNAL_H} > task_sig.h

parser.h parser.c: parser.y
		rm -f parser.c parser.h
		$(YACC) $(YFLAGS) parser.y
		mv y.tab.c parser.c
		mv y.tab.h parser.h

lexer.c: lexer.l
		rm -f lexer.c
		${LEX} ${LFLAGS} lexer.l
		mv lex.yy.c lexer.c

version.c:	${SRCS} ${INCLUDES} version.awk
		-for i in ${SRCS} ${INCLUDES} ;\
		do \
			${HEAD} $$i ; \
		done | ${AWK} -f version.awk - > version.c
		echo 'char *build_date = "'`date`'";' >> version.c

install:	all
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) in.gated
		$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) ripquery
		$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) gated.bgp
		$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) gated.egp
		$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) gated.hello
		$(INS) -f $(CONFDIR) -m 0444 -u $(OWN) -g $(GRP) gated.rip

clean:
		rm -f $(OBJS) task_sig.h y.tab.* y.output lexer.c parser.c \
		parser.h version.c ripquery.o yacc.acts yacc.debug yacc.tmp

clobber:	clean
		rm -f in.gated ripquery

lintit:
		$(LINT) $(LINTFLAGS) *.c

FRC:
