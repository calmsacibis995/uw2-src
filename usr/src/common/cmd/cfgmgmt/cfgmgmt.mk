#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cfgmgmt:cfgmgmt.mk	1.8.4.2"
#ident "$Header: cfgmgmt.mk 2.0 91/07/23 $"

include $(CMDRULES)

OAMBASE		= $(ROOT)/$(MACH)/usr/sadm/sysadm
BINDIR		= $(OAMBASE)/bin
DESTDIR		= $(OAMBASE)/menu/machinemgmt/configmgmt
HELPSRCDIR	= .

SHFILES		=
FMTFILES	=
DISPFILES	= cfgmgmt.menu
HELPFILES	= Help
OWN		= bin
GRP		= bin


all: $(SHFILES) $(HELPFILES) $(FMTFILES) $(DISPFILES) 

# $(SHFILES):

$(HELPFILES):

# $(FMTFILES):

$(DISPFILES):

clean:

clobber:	clean

lintit:

size:

strip:

install: $(DESTDIR) all
#	for i in $(SHFILES) ;\
#	do \
#		$(INS) -m 640 -g $(GRP) -u $(OWN) -f $(DESTDIR) $$i ;\
#	done
	for i in $(HELPFILES) ;\
	do \
		$(INS) -m 640 -g $(GRP) -u $(OWN) -f $(DESTDIR) $(HELPSRCDIR)/$$i ;\
	done
#	for i in $(FMTFILES) ;\
#	do \
#		$(INS) -m 640 -g $(GRP) -u $(OWN) -f $(DESTDIR) $$i ;\
#	done
	for i in $(DISPFILES) ;\
	do \
		$(INS) -m 640 -g $(GRP) -u $(OWN) -f $(DESTDIR) $$i ;\
	done

$(DESTDIR):
	builddir() \
	{ \
		if [ ! -d $$1 ]; \
		then \
		    builddir `dirname $$1`; \
		    mkdir $$1; \
		fi \
	}; \
	builddir $(DESTDIR)
