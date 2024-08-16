#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:net/isocket/isocket.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	isocket.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/isocket

ISOCKET = isocket.cf/Driver.o
LFILE = $(LINTDIR)/isocket.ln

FILES = \
	isocket.o

CFILES = \
	isocket.c

SRCFILES = $(CFILES)

LFILES = \
	isocket.ln

all: $(ISOCKET)

install: all
	(cd isocket.cf; $(IDINSTALL) -R$(CONF) -M isocket)

$(ISOCKET): $(FILES)
	$(LD) -r -o $(ISOCKET) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(ISOCKET)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e isocket

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	isocket.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
