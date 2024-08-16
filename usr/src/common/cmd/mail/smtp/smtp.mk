#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/smtp/smtp.mk	1.4.3.17"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/mail/smtp/smtp.mk,v 1.19.4.1 1994/12/14 19:02:35 sharriso Exp $"

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
LDLIBS= -L$(TOOLS)/usr/lib -lmail -lserver -lresolv -lsocket -lnsl

LOCALINC= -I ../../../head

OBJS = \
	connection.o \
	getMx.o \
	log.o \
	main.o \
	message.o \
	msgConn.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f smtp.mk smtp smtpqer

smtp: $(OBJS)
	$(CC) -O -o $@ $(OBJS) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

smtpqer: smtpqer.o checkMx.o log.o
	$(CC) -o $@ smtpqer.o checkMx.o log.o $(LDFLAGS) -lmail -lresolv  -lnsl $(PERFLIBS)

install: all
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 06555 -u smtp -g mail smtp
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 06555 -u smtp -g mail smtpqer
	
clean:
	rm -f *.o *.lerr *.ln
	
clobber: clean
	rm -f smtp smtpqer smtp.lint smtpqer.lint

lintit: smtp.lint smtpqer.lint

smtp.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

smtpqer.lint: smtpqer.ln
	$(LINT) $(LINTFLAGS) smtpqer.ln > $@


checkMx.ln checkMx.o: smtp.h
connection.ln connection.o: smtpType.h smtp.h
getMx.ln getMx.o: smtpType.h smtp.h
main.ln main.o: smtpType.h smtp.h
message.ln message.o: smtpType.h smtp.h
msgConn.ln msgConn.o: smtpType.h smtp.h
smtpqer.ln smtpqer.o: smtp.h

localinstall: smtp smtpqer
	$(INS) -f /usr/lib/mail/surrcmd -m 06555 -u smtp -g mail smtp
	$(INS) -f /usr/lib/mail/surrcmd -m 06555 -u smtp -g mail smtpqer
	/usr/sbin/filepriv -f setuid /usr/lib/mail/surrcmd/smtp
	/usr/sbin/filepriv -f setuid /usr/lib/mail/surrcmd/smtpqer
