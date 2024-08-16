#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)lp:filter/Makefile	1.15.2.9"
#ident "$Header: $"
#
# Makefile for lp/filter
#



include $(CMDRULES)

TOP	=	..

include ../common.mk


MISC	= \
		slow.filter \
		pcl \
		I18N_pp \
		I18N_print \
		I18N_wrap


all:		$(MISC)
	cd postscript; $(MAKE) $(MAKEARGS) all
	cd pcldir; $(MAKE) $(MAKEARGS) all
	cd i18n; $(MAKE) $(MAKEARGS) all
	cd wrap; $(MAKE) $(MAKEARGS) all
	cd lpd; $(MAKE) $(MAKEARGS) all

install:	all
	for misc in $(MISC); \
	do \
		$(INS) -m $(EMODES) -u $(OWNER) -g $(GROUP) -f $(LPBINDIR) $$misc; \
	done
	cd postscript; $(MAKE) $(MAKEARGS) install
	cd pcldir; $(MAKE) $(MAKEARGS) install
	cd i18n; $(MAKE) $(MAKEARGS) install
	cd wrap; $(MAKE) $(MAKEARGS) install
	cd lpd; $(MAKE) $(MAKEARGS) install

clean clobber:
	cd postscript; $(MAKE) $(MAKEARGS) $@
	cd pcldir; $(MAKE) $(MAKEARGS) $@
	cd i18n; $(MAKE) $(MAKEARGS) $@
	cd wrap; $(MAKE) $(MAKEARGS) $@
	cd lpd; $(MAKE) $(MAKEARGS) $@

pcl:
	cp pcldir/pcl.filter pcl

I18N_pp:
	cp i18n/I18N_pp.filter I18N_pp

I18N_print:
	cp i18n/I18N_print.filter I18N_print

I18N_wrap:
	cp wrap/I18N_wrap.filter I18N_wrap

strip:

lintit:

lintlib:
