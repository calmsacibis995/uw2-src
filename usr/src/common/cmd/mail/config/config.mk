#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/config/config.mk	1.9"
#ident "$Header: /SRCS/esmp/usr/src/q7/cmd/mail/config/config.mk,v 1.2 1994/08/29 18:02:24 sharriso Exp $"

#	Makefile for config 

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
#
#	Some of these are Perl files and/or data files.
#
MAINS = 	configCheck configFiles createSurr mailsurr.proto mailflgs


OBJECTS =  createSurr.o

LINTFILES = $(OBJECTS:.o=.ln)

all:
	$(MAKE) -f config.mk configCheck configFiles createSurr mailsurr.proto config.files

install: all $(USRLIB)/mail $(USRLIB)/mail/surrcmd
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail -m 0644 -u bin -g bin mailsurr.proto
	$(INS) -f $(ROOT)/$(MACH)/etc/mail -m 0644 -u bin -g bin config.files
	$(INS) -f $(ROOT)/$(MACH)/etc/mail -m 0664 -u mail -g mail mailflgs
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 0555 -u bin -g bin configCheck
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 0555 -u bin -g bin configFiles
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 04555 -u root -g bin createSurr
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 0555 -u bin -g bin doUucpTcp


createSurr:		createSurr.o 
	$(CC) -o $@ $@.o $(LDFLAGS) $(PERFLIBS)

clean:
	rm -f $(OBJECTS) $(LINTFILES) *.lerr

clobber: clean
	rm -f createSurr createSurr.lint

localinstall: mailsurr.proto configCheck configFiles createSurr
	$(INS) -f /usr/lib/mail -m 0644 -u bin -g bin mailsurr.proto
	$(INS) -f /etc/mail -m 0644 -u bin -g bin config.files
	$(INS) -f /etc/mail -m 0664 -u mail -g mail mailflgs
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u bin -g bin configCheck
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u bin -g bin configFiles
	$(INS) -f /usr/lib/mail/surrcmd -m 04555 -u root -g bin createSurr
	/usr/sbin/filepriv -f setuid /usr/lib/mail/surrcmd/createSurr
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u bin -g bin doUucpTcp


lintit: createSurr.lint

createSurr.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

$(USRLIB)/mail/surrcmd $(USRLIB)/mail:
	mkdir -p $@
