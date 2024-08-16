#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)langsup:common/ls/X/fonts/misc/fonts.mk	1.4"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

include $(CMDRULES)

SRCDIR = .
MISCDIR = $(PKGDIR)/usr/X/lib/fonts/misc
BDFTOSNF=$(ROOT)/$(MACH)/usr/X/bin/bdftosnf
LBDFTOSNF=/usr/X/bin/bdftosnf
ELSVAR=$(PKGDIR)/var/opt/ls

SRCS	= 7x14rk k14

all:

install: $(SRCS) fonts.alias.ls font.list
	if [ ! -d $(MISCDIR) ] ; then mkdir -p $(MISCDIR) ; fi
	for i in $(SRCS); do\
		install -f $(MISCDIR) -m 0644 -u bin -g bin -s $$i.snf;\
	done
	install -f $(MISCDIR) -m 0755 -u bin -g bin -s fonts.alias.ls;
	install -f $(ELSVAR) -m 0755 -u bin -g bin -s font.list
	rm -f *.snf

$(SRCS):
	if [ -x $(BDFTOSNF) ] ; \
		then $(BDFTOSNF) -L -l -t $@.bdf > $@.snf ; \
		else $(LBDFTOSNF) -L -l -t $@.bdf > $@.snf ; fi

clean:
	rm -f *.snf

