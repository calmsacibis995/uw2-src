#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/ict/ict.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	ict.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/ict

ICT = ict.cf/Driver.o
ICTSTUB = ict.cf/Modstub.o
LFILE = $(LINTDIR)/ict.ln

ASFLAGS = -m

FILES = ict.o
CFILES = ict.c
LFILES = ict.ln

SRCFILES = $(CFILES)

.s.o:
	$(AS) -m $<

all:	$(ICT)	$(ICTSTUB)

install:	all
		( \
		cd ict.cf ; $(IDINSTALL) -R$(CONF) -M ict; \
		rm -f  $(CONF)/pack.d/ict/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/ict/ \
		)


$(ICT):	$(FILES)
		$(LD) -r -o $(ICT) $(FILES)

$(ICTSTUB):	ict_stub.o
		$(LD) -r -o $(ICTSTUB) ict_stub.o
clean:
	-rm -f *.o $(LFILES) *.L

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e ict

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
	ict.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
