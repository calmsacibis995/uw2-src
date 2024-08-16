#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/mcst/mcst.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	mcst.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/mcst

MCST = mcst.cf/Driver.o
MCSTSTUB = mcst.cf/Modstub.o
LFILE = $(LINTDIR)/mcst.ln

ASFLAGS = -m

FILES = mcst.o
CFILES = mcst.c
LFILES = mcst.ln

SRCFILES = $(CFILES)

.s.o:
	$(AS) -m $<

all:	$(MCST) $(MCSTSTUB)

install:	all
		( \
		cd mcst.cf ; $(IDINSTALL) -R$(CONF) -M mcst; \
		rm -f $(CONF)/pack.d/mcst/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/mcst/  \
		)

$(MCST):	$(FILES)
		$(LD) -r -o $(MCST) $(FILES)

$(MCSTSTUB):	mcst_stub.o
		$(LD) -r -o $(MCSTSTUB) mcst_stub.o

clean:
	-rm -f *.o $(LFILES) *.L $(MCST) $(MCSTSTUB)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e mcst

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
	mcst.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
