#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/loopback/ticots/ticots.mk	1.7"
#ident 	"$Header: $"

include $(UTSRULES)

MAKEFILE=	ticots.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/loopback/ticots
LFILE = $(LINTDIR)/ticots.ln

TICOTS = ticots.cf/Driver.o
TICOTSRC = ticots.c
TICOTSOBJ= ticots.o
TICOTSLN = ticots.ln

TICOTSORD = ticotsor.cf/Driver.o
TICOTSORDSRC = ticotsord.c
TICOTSORDOBJ = ticotsord.o
TICOTSORDLN = ticotsord.ln

CFILES = ticots.c ticotsord.c
LFILES = \
	ticots.ln \
	ticotsord.ln

SRCFILES = $(CFILES)

TCOOSED = /bin/sed -e s/ticots/ticotsord/g -e s/tco/tcoo/g -e s/TCO/TCOO/g

all: $(TICOTS) $(TICOTSORD)

install: all
	(cd ticots.cf; $(IDINSTALL) -R$(CONF) -M ticots)
	(cd ticotsor.cf; $(IDINSTALL) -R$(CONF) -M ticotsor)

$(TICOTS): $(TICOTSOBJ)
	$(LD) -r -o $(TICOTS) $(TICOTSOBJ)

$(TICOTSORD): $(TICOTSORDOBJ)
	$(LD) -r -o $(TICOTSORD) $(TICOTSORDOBJ)

$(TICOTSORDSRC):
	$(TCOOSED) <$(TICOTSRC) >$@

$(TICOTSORDOBJ): $(TICOTSORDSRC)
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -DTICOTSORD -c $(TICOTSORDSRC)

$(TICOTSOBJ):
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -DTICOTS -c $(TICOTSRC)

clean:
	-rm -f *.o $(LFILES) *.L ticotsord.c

clobber: clean
	-rm -f $(TICOTS) $(TICOTSORD)
	-rm -f $(LINTDIR)/*
	$(IDINSTALL) -e -R$(CONF) -d -e ticots
	$(IDINSTALL) -e -R$(CONF) -d -e ticotsor

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE):	$(LINTDIR) $(TICOTSLN) $(TICOTSORDLN)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L

$(TICOTSLN):
	echo "\n$(DIR)/`basename $@ .ln`.c:" > `basename $@ .ln`.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) -DTICOTS \
	-c -u `basename $@ .ln`.c >> `basename $@ .ln`.L
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	cat $@ >> $(LFILE)
	cat `basename $@ .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L

$(TICOTSORDLN):	$(TICOTSORDSRC)
	echo "\n$(DIR)/`basename $@ .ln`.c:" > `basename $@ .ln`.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) -DTICOTSORD \
	-c -u `basename $@ .ln`.c >> `basename $@ .ln`.L
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	cat $@ >> $(LFILE)
	cat `basename $@ .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done


FRC:

headinstall:


include $(UTSDEPEND)

include $(MAKEFILE).dep
