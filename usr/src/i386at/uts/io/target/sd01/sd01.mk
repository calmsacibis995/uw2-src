#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/target/sd01/sd01.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	sd01.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/sd01

SD01 = sd01.cf/Driver.o
LFILE = $(LINTDIR)/sd01.ln

FILES = sd01.o sd01flt.o
CFILES = sd01.c sd01flt.c
LFILES = sd01.ln

SRCFILES = $(CFILES)

all:	$(SD01)

install:	all
		(cd sd01.cf ; $(IDINSTALL) -R$(CONF) -M sd01)

$(SD01):	$(FILES)
		$(LD) -r -o $(SD01) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SD01)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e sd01

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
	fdisk.h \
	sd01.h \
	sd01_ioctl.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
