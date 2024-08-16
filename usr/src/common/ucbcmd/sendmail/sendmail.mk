#ident	"@(#)ucb:common/ucbcmd/sendmail/sendmail.mk	1.8"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#



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
#	Makefile for sendmail base directory
#

include $(CMDRULES)

INSDIR1 = $(USR)/ucb

INSDIR = $(USR)/ucblib

MAILDIR = $(ETC)/ucbmail

ALL = src/sendmail 

DIRS = $(VAR)/spool/mqueue

SENDMAIL=$(USR)/ucblib/sendmail

all: $(MAKEFILES)
	cd lib; $(MAKE)  all
	cd src; $(MAKE)  all
	cd aux; $(MAKE)  all
	cd cf;  $(MAKE)  all

clean:
	cd lib;	$(MAKE)  clean
	cd src;	$(MAKE)  clean
	cd aux;	$(MAKE)  clean
	cd cf;  $(MAKE)  clean
clobber:
	cd lib;	$(MAKE)  clobber
	cd src;	$(MAKE)  clobber
	cd aux;	$(MAKE)  clobber
	cd cf;  $(MAKE)  clobber

install: all $(ALL) $(DIRS)
	cd src; $(MAKE)  install #DESTDIR=${DESTDIR} install
	[ -d $(MAILDIR) ] || mkdir -p $(MAILDIR)
	$(INS) -f $(MAILDIR) -m 444 lib/sendmail.hf 
#	cp  -m 660	/dev/null	$(SENDMAIL).fc
	$(INS) -f $(MAILDIR) -m 444 cf/subsidiary.cf 
	$(INS) -f $(MAILDIR) -m 444 cf/main.cf 
	-ln cf/subsidiary.cf cf/sendmail.cf
	$(INS) -f $(MAILDIR) -m 444 cf/sendmail.cf 
	$(SYMLINK) $(MAILDIR)/sendmail.cf $(ETC)/sendmail.cf
	$(SYMLINK) $(MAILDIR)/sendmail.cf $(INSDIR)/sendmail.cf
	-rm -f cf/sendmail.cf
	$(INS) -f $(MAILDIR) lib/aliases
	$(SYMLINK) $(MAILDIR)/aliases $(INSDIR)/aliases
	$(INS) -f $(INSDIR1) aux/vacation
	$(INS) -f $(INSDIR1) aux/mconnect
	$(INS) -f $(INSDIR1) aux/mailstats
	-rm -f $(INSDIR1)/newaliases
	-rm -f $(INSDIR1)/mailq
	$(SYMLINK) $(INSDIR)/sendmail $(INSDIR1)/newaliases
	$(SYMLINK) $(INSDIR)/sendmail $(INSDIR1)/mailq
	-mv $(MAILDIR)/mailsurr $(MAILDIR)/mailsurr.`/bin/sh -c "echo $$$$"`	
	$(INS) -f $(MAILDIR) cf/mailsurr

$(DIRS):
	[ -d $(DIRS) ] || mkdir -p $(DIRS)
	$(SYMLINK) $(DIRS) $(USR)/ucblib/mqueue
