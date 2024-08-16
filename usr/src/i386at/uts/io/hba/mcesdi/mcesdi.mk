#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/mcesdi/mcesdi.mk	1.6"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	mcesdi.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/mcesdi

MCESDI = mcesdi.cf/Driver.o
MCESDISTUB = mcesdi.cf/Modstub.o
LFILE = $(LINTDIR)/mcesdi.ln

ASFLAGS = -m

FILES = mcesdi.o
CFILES = mcesdi.c
LFILES = mcesdi.ln

SRCFILES = $(CFILES)

.s.o:
	$(AS) -m $<

all:	$(MCESDI) $(MCESDISTUB)

install:	all
		( \
		cd mcesdi.cf ; $(IDINSTALL) -R$(CONF) -M mcesdi; \
		rm -f $(CONF)/pack.d/mcesdi/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/mcesdi/  \
		)

$(MCESDI):	$(FILES)
		$(LD) -r -o $(MCESDI) $(FILES)

$(MCESDISTUB):	mcesdi_stub.o
		$(LD) -r -o $(MCESDISTUB) mcesdi_stub.o

clean:
	-rm -f *.o $(LFILES) *.L $(MCESDI) $(MCESDISTUB)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e mcesdi

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
	mcesdi.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
