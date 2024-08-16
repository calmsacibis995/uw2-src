#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:io/tp/tp.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	tp.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/tp

LOCALDEF = -DDDI_OFF

TP = tpath.cf/Driver.o
LFILE = $(LINTDIR)/tp.ln
MODSTUB = tpath.cf/Modstub.o

FILES = \
	tp.o 

CFILES = \
	tp.c 

LFILES = \
	tp.ln

all: $(TP) $(MODSTUB)

install: all
	(cd tpath.cf; $(IDINSTALL) -R$(CONF) -M tpath)

$(TP): $(FILES)
	$(LD) -r -o $(TP) $(FILES)

$(MODSTUB): tpath_stub.o
	$(LD) -r -o $@ tpath_stub.o 

clean:
	-rm -f *.o $(LFILES) *.L $(TP) $(MODSTUB)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e tpath

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
	@for i in $(CFILES); do \
		echo $$i; \
	done

sysHeaders = \
	tp.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done



include $(UTSDEPEND)

include $(MAKEFILE).dep
