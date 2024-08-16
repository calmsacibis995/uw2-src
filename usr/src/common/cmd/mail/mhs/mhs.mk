#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/mhs/mhs.mk	1.15"
#ident "$Header: /SRCS/esmp/usr/src/nw/cmd/mail/mhs/mhs.mk,v 1.21.2.1 1994/10/28 22:32:40 sharriso Exp $"

#	Makefile for MHS handling for /bin/mail

include $(CMDRULES)
LOCALE = $(USRLIB)/locale/C/MSGFILES

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
SURRDIR =	$(USRLIB)/mail/surrcmd
SUIDDIR =	$(SURRDIR)/suid

OWN = bin
GRP = bin

LDLIBS =

#
#	Some of these are Perl files and/or data files.
#
MAINS = 	decrypt getDomain smf-in smf-out.suid smf-poll suidprog smfqueue smfsched
MSGS = \
    msgs_output/smf-out \
    msgs_output/smf-in \
    msgs_output/smf-poll

OBJECTS =  decrypt.o suidprog.o

all:		$(MAINS) $(MSGS)
	cd applets; make -f apps.mk all

install: all suidprog_inst smf-out_inst $(LOCALE) $(SURRDIR)
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) decrypt
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) keyVal
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) mhsConfig
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) splitAddr
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) getDomain
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) smf-in
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) smf-poll
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) smfsched
	$(INS) -f $(SURRDIR) -m 0555 -u $(OWN) -g $(GRP) smfqueue
	cd msgs_input; $(INS) -f $(LOCALE) -m 0555 -u $(OWN) -g $(GRP) smf-poll
	cd msgs_input; $(INS) -f $(LOCALE) -m 0555 -u $(OWN) -g $(GRP) smf-in
	cd msgs_input; $(INS) -f $(LOCALE) -m 0555 -u $(OWN) -g $(GRP) smf-out
	cd applets; make -f apps.mk install

$(LOCALE):
	mkdir -p $@

suidprog_inst: suidprog $(SURRDIR)
	rm -f smf-out
	cp suidprog smf-out
	$(INS) -f $(SURRDIR) -m 4555 -u mhsmail -g mail smf-out

smf-out_inst: smf-out.suid $(SURRDIR) $(SUIDDIR)
	rm -f smf-out
	cp smf-out.suid smf-out
	$(INS) -f $(SUIDDIR) -m 0555 -u $(OWN) -g $(GRP) smf-out

decrypt:		decrypt.o 
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

suidprog:		suidprog.o 
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

smfqueue.o:	smf.h
smfqueue:		smfqueue.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

smfsched.o:	smf.h
smfsched:		smfsched.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

msgs_output/smf-in: msgs_input/smf-in
	[ -d msgs_output ] || mkdir msgs_output
	mkmsgs -o msgs_input/$(@F) $@

msgs_output/smf-poll: msgs_input/smf-poll
	[ -d msgs_output ] || mkdir msgs_output
	mkmsgs -o msgs_input/$(@F) $@

msgs_output/smf-out: msgs_input/smf-out
	[ -d msgs_output ] || mkdir msgs_output
	mkmsgs -o msgs_input/$(@F) $@

clean:
	cd applets; make -f apps.mk clean
	rm -f $(OBJECTS) *.ln *.lerr

clobber:
	cd applets; make -f apps.mk clobber
	rm -f $(OBJECTS) smf-out decrypt suidprog decrypt.lint suidprog.lint $(MSGS)

lintit: decrypt.lint suidprog .lint smfqueue.lint smfsched.lint

decrypt.lint: decrypt.ln
	$(LINT) $(LINTFLAGS) decrypt.ln > $@

suidprog.lint: suidprog.ln
	$(LINT) $(LINTFLAGS) suidprog.ln > $@

smfqueue.lint: smfqueue.ln
	$(LINT) $(LINTFLAGS) suidprog.ln > $@

smfsched.lint: smfsched.ln
	$(LINT) $(LINTFLAGS) suidprog.ln > $@

localinstall: decrypt getDomain smf-in smf-poll suidprog_localinst smf-out_localinst smfsched smfqueue
	cd applets; make -f apps.mk localinstall
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) decrypt
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) getDomain
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) smf-in
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) smf-poll
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) smfsched
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) smfqueue
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) keyVal
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) mhsConfig
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u $(OWN) -g $(GRP) splitAddr

suidprog_localinst: suidprog
	rm -f smf-out
	cp suidprog smf-out
	$(INS) -f /usr/lib/mail/surrcmd -m 4555 -u mhsmail -g $(GRP) smf-out

smf-out_localinst: /usr/lib/mail/surrcmd/suid smf-out.suid
	rm -f smf-out
	cp smf-out.suid smf-out
	$(INS) -f /usr/lib/mail/surrcmd/suid -m 0555 -u mhsmail -g $(GRP) smf-out

$(SUIDDIR) $(SURRDIR) /usr/lib/mail/surrcmd/suid:
	mkdir -p $@
	$(CH)chown bin $@
	$(CH)chgrp bin $@
	$(CH)chmod 775 $@
