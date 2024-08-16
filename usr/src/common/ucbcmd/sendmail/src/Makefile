#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/sendmail/src/Makefile	1.7.2.3"
#ident  "$Header: $"


#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.
#

#  This Makefile is designed to work on the old "make" program.  It does
#  not use the obj subdirectory.  It also does not install documentation
#  automatically -- think of it as a quick start for sites that have the
#  old make program (I recommend that you get and port the new make if you
#  are going to be doing any signficant work on sendmail).
#
#	Based on a Makefile for Dell SVR4 Issue 2.2 from Kimmo Suominen
#	<kim@grendel.lut.fi> -- I haven't tested this myself.  It may
#	work on other SVR4 ports.
#
#	@(#)Makefile.SVR4	8.3 (Berkeley) 4/11/94
#

include $(CMDRULES)

INSDIR = $(ROOT)/$(MACH)/usr/ucblib

INC1 = $(ROOT)/$(MACH)/usr/ucbinclude

#LOCALINC = -I$(INC1) -I../include
LOCALINC = -I../include

# make sure the shell constructs below use the right shell
SHELL=	/bin/sh

# use O=-O (usual) or O=-g (debugging)
O=	-O

DESTDIR=$(ROOT)/$(MACH)

# define the database mechanism used for alias lookups:
#	-DNDBM -- use new DBM
#	-DNEWDB -- use new Berkeley DB
#	-DNIS -- include NIS support
# The really old (V7) DBM library is no longer supported.
# See READ_ME for a description of how these flags interact.
#
#DBMDEF=	-DNEWDB -DNDBM
DBMDEF= -DNIS -DNDBM -DHASFLOCK=0

# environment definitions (e.g., -D_AIX3)
ENVDEF=	-DUNIXWARE

# see also conf.h for additional compilation flags

# library directories
LIBDIRS=

# libraries required on your system
#LIBS=	-ldb -ldbm -lresolv -lsocket -lnsl -lelf
LIBS=	-lresolv -lsocket -lnsl -lelf -lgen -lc $(ROOT)/$(MACH)/usr/ucblib/libucb.a

# location of sendmail binary (usually /usr/sbin or /usr/lib)
BINDIR=	${DESTDIR}/usr/ucblib

# location of sendmail.st file (usually /var/log or /usr/lib)
STDIR=	${DESTDIR}/usr/ucblib

# location of sendmail.hf file (usually /usr/share/misc or /usr/lib)
HFDIR=	${DESTDIR}/usr/ucblib

# additional .o files needed
OBJADD=

###################  end of user configuration flags  ######################

CFLAGS=	-I. $O ${LOCALINC} ${DBMDEF} ${ENVDEF}

OBJS=	alias.o arpadate.o clock.o collect.o conf.o convtime.o daemon.o \
	deliver.o domain.o envelope.o err.o headers.o macro.o main.o \
	map.o mci.o parseaddr.o queue.o readcf.o recipient.o \
	savemail.o srvrsmtp.o stab.o stats.o sysexits.o \
	trace.o udb.o usersmtp.o util.o version.o ${OBJADD}

LINKS=	${DESTDIR}/usr/ucb/newaliases ${DESTDIR}/usr/ucb/mailq
BINOWN=	root
BINGRP=	mail
BINMODE=6555
INSTALL=/usr/ucb/install

ALL=	sendmail

all: ${ALL}

sendmail: ${BEFORE} ${OBJS}
	${CC} -o sendmail ${OBJS} ${LIBDIRS} ${LIBS}

NROFF=	nroff -h

aliases.0: aliases.5
	${NROFF} -mandoc aliases.5 > aliases.0

mailq.0: mailq.1
	${NROFF} -mandoc mailq.1 > mailq.0

newaliases.0: newaliases.1
	${NROFF} -mandoc newaliases.1 > newaliases.0

sendmail.0: sendmail.8
	${NROFF} -mandoc sendmail.8 > sendmail.0

install: all
	$(INS) -f $(INSDIR) -m 4551 -u $(OWN) sendmail

install-sendmail: sendmail
	${INSTALL} -o ${BINOWN} -g ${BINGRP} -m ${BINMODE} sendmail ${BINDIR}
	for i in ${LINKS}; do rm -f $$i; ln -s ${BINDIR}/sendmail $$i; done
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 644 /dev/null \
	    ${STDIR}/sendmail.st
	${INSTALL} -c -o ${BINOWN} -g ${BINGRP} -m 444 sendmail.hf ${HFDIR}

# doesn't actually install them -- you may want to install pre-nroff versions
install-docs: aliases.0 mailq.0 newaliases.0 sendmail.0

clean:
	rm -f *.o

clobber: clean
	rm -f ${OBJS} sendmail aliases.0 mailq.0 newaliases.0 sendmail.0


# dependencies
#   gross overkill, and yet still not quite enough....
${OBJS}: sendmail.h conf.h

map.o: map.c
	$(CC) $(CFLAGS) -I$(INC) -I$(INC1) -c map.c
