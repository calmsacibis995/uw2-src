#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/smtpd/smtpd.mk	1.7"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/mail/smtpd/smtpd.mk,v 1.13 1994/07/27 19:34:36 eric Exp $"

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
LOCALINC=-I../../../head
LDLIBS= -L$(TOOLS)/usr/lib -lmail -lserver -lnsl
LINTFLAGS=-u

OBJS = \
	blnk.o \
	command.o \
	connection.o \
	data.o \
	expn.o \
	helo.o \
	help.o \
	log.o \
	mail.o \
	main.o \
	message.o \
	noop.o \
	quit.o \
	rcpt.o \
	rset.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f smtpd.mk smtpd mailWrapper

smtpd: $(OBJS)
	$(CC) -g -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

mailWrapper: mailWrapper.o
	$(CC) -g -o $@ mailWrapper.o

install: $(ETC)/init.d all
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 04555 -u smtp -g mail smtpd
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 0555 -u smtp -g mail mailWrapper
	$(INS) -f $(ROOT)/$(MACH)/etc/init.d -m 0555 -u smtp -g mail smtpinit
	
clean:
	rm -f *.o *.lerr
	
clobber: clean
	rm -f smtpd mailWrapper lintit

lintit: smtpd.lint mailWrapper.lint

smtpd.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

mailWrapper.lint: mailWrapper.ln
	$(LINT) $(LINTFLAGS) mailWrapper.ln >$@

blnk.ln blnk.o: smtp.h smtpState.h
command.ln command.o: smtp.h smtpState.h help.h
connection.ln connection.o: smtp.h smtpState.h
data.ln data.o: smtp.h smtpState.h
expn.ln expn.o: smtp.h smtpState.h
helo.ln helo.o: smtp.h smtpState.h
help.ln help.o: smtp.h smtpState.h
mail.ln mail.o: smtp.h smtpState.h
main.ln main.o: smtp.h smtpState.h
message.ln message.o: smtp.h smtpState.h
log.ln log.o: smtp.h smtpState.h
noop.ln noop.o: smtp.h smtpState.h
quit.ln quit.o: smtp.h smtpState.h
rcpt.ln rcpt.o: smtp.h smtpState.h
rset.ln rset.o: smtp.h smtpState.h

localinstall: smtpd mailWrapper
	$(INS) -f /usr/lib/mail/surrcmd -m 06555 -u smtp -g mail smtpd
	filepriv -f setuid,filesys /usr/lib/mail/surrcmd/smtpd
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u smtp -g mail mailWrapper
	$(INS) -f $(ROOT)/$(MACH)/etc/init.d -m 0555 -u smtp -g mail smtpinit

$(ETC)/init.d:
	mkdir -p $@
	$(CH)chown root $@
	$(CH)chgrp sys $@
	$(CH)chmod 0775 $@
