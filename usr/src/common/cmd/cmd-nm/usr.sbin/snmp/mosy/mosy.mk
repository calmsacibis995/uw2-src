#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nm:common/cmd/cmd-nm/usr.sbin/snmp/mosy/mosy.mk	1.6"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nm/usr.sbin/snmp/mosy/mosy.mk,v 1.6 1994/06/24 16:11:26 rbell Exp $"
#
# Copyrighted as an unpublished work.
# (c) Copyright 1992 INTERACTIVE Systems Corporation
# All rights reserved.
#
#      @(#)Makefile

include $(CMDRULES)

LOCALDEF = -DSVR4
LOCALINC = -I$(INC)/netmgt
OBJS	=	asp.o mosy.o misc.o yacc.o
LDLIBS	=	-lsnmp -lm

all:		mosy

mosy:		asp.o mosy.o misc.o yacc.o
		${CC} -o mosy.dy ${LDFLAGS} ${OBJS} ${LDLIBS}
		${CC} -o mosy ${LDFLAGS} ${OBJS} ${LDLIBS} -dn

asp.o:		asp.c mosy-defs.h
mosy.o:		mosy.c mosy-defs.h
misc.o:		misc.c mosy-defs.h
yacc.o:		yacc.c lex.include mosy-defs.h

yacc.c:		yacc.y
		-@echo "expect 23 shift/reduce and 11 reduce/reduce conflicts"
		yacc $(YACCFLAGS) yacc.y
		mv y.tab.c $@

lex.include:	lex.l
		$(LEX) $(LEXFLAGS) lex.l
		mv lex.yy.c $@

install:	all
		${INS} -f ${USRSBIN} -m 0555 -u ${OWN} -g ${GRP} mosy
		${INS} -f ${USRSBIN} -m 0555 -u ${OWN} -g ${GRP} mosy.dy
	[ -d ${USRLIB}/locale/C/MSGFILES ] || \
		mkdir -p ${USRLIB}/locale/C/MSGFILES
	${INS} -f ${USRLIB}/locale/C/MSGFILES -m 0666 -u ${OWN} -g ${GRP} nmmosy.str
		
clean:
		rm -f $(OBJS) yacc.c lex.include *~

clobber:	clean
		rm -f mosy
