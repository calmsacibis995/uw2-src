#ident	"@(#)es_le:common/le/es/build/misc.mk	1.3"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= es

XOLSRC = ../runtime/usr/X/lib/locale/$(LOCALE)/messages

XOLFILES = xol_msgs xol_errs

XOLDIR = $(ROOT)/$(MACH)/usr/X/lib/locale/$(LOCALE)/messages

XLOCSRC = ../runtime/usr/X/lib/locale/$(LOCALE)

MWMFILE = system.mwmrc

UPFILES = README.UW2.0.sys README.UW2.0.user

XLOCDIR = $(ROOT)/$(MACH)/usr/X/lib/locale/$(LOCALE)

all:

install: $(XOLSRC)
	[ -d $(XOLDIR) ] || mkdir -p $(XOLDIR)
	for i in $(XOLFILES) ;\
	do \
		$(INS) -f $(XOLDIR) $(XOLSRC)/$$i ;\
	done
	for i in $(MWMFILE) $(UPFILES) ;\
	do \
		$(INS) -f $(XLOCDIR) $(XLOCSRC)/$$i ;\
	done
	
clean:

clobber:

