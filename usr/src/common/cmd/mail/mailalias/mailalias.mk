#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/mailalias/mailalias.mk	1.1"
#	Makefile for mailalias

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr

LDLIBS = -lmail

OBJS =  mailalias.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f mailalias.mk clobber mailalias

install: all $(ETC)/mail $(ETC)/mail/lists
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 0555 -u bin -g bin mailalias
	$(INS) -f $(ROOT)/$(MACH)/etc/mail -m 0644 -u bin -g bin namefiles
	$(INS) -f $(ROOT)/$(MACH)/etc/mail -m 0644 -u bin -g bin names


mailalias:		$(OBJS) 
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f mailalias

localinstall: mailalias
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u bin -g bin mailalias

lintit: mailalias.lint

mailalias.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

$(ETC)/mail/lists:
	mkdir -p $@
	$(CH)chown bin $@
	$(CH)chgrp mail $@
	$(CH)chmod 0775 $@

$(ETC)/mail:
	mkdir -p $@
	$(CH)chown bin $@
	$(CH)chgrp mail $@
	$(CH)chmod 0775 $@
