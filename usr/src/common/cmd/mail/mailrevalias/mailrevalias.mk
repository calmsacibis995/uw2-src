#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)mail:common/cmd/mail/mailrevalias/mailrevalias.mk	1.2"
#ident "$Header: /SRCS/esmp/usr/src/q7/cmd/mail/mailrevalias/mailrevalias.mk,v 1.2 1994/08/29 18:02:36 sharriso Exp $"

#	Makefile for mailrevalias

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
LDLIBS = -L$(CCSLIB) -lmail

OBJS =  mailrevalias.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f mailrevalias.mk clobber mailrevalias

install: all
	$(INS) -f $(ROOT)/$(MACH)/usr/lib/mail/surrcmd -m 0555 -u bin -g bin mailrevalias


mailrevalias:		$(OBJS) 
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(PERFLIBS)

clean:
	rm -f *.o *.ln *.lerr

clobber: clean
	rm -f mailrevalias mailrevalias.lint

localinstall: mailrevalias
	$(INS) -f /usr/lib/mail/surrcmd -m 0555 -u bin -g bin mailrevalias

lintit: mailrevalias.lint

mailrevalias.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@
