#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/async/async.mk	1.6"
#ident	"$Header: $"

#include ./uts.rulefile
include $(UTSRULES)

MAKEFILE=	async.mk
KBASE     = ../..
DIR = io/async
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/async.ln
AIO = async.cf/Driver.o
MODSTUB = async.cf/Modstub.o

FILES = async.o
CFILES = async.c
LFILES = async.ln
SRCFILES = $(CFILES)

GRP = bin
OWN = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)

HEADERS = aiosys.h aio_hier.h

all:	$(AIO) $(MODSTUB)
install: all
	cd async.cf; $(IDINSTALL) -R$(CONF) -M async

$(AIO): $(FILES)
	$(LD) -r -o $@ $(FILES)

$(MODSTUB): async_stub.o
	$(LD) -r -o $@ async_stub.o

headinstall:	
	@for i in $(HEADERS); \
	do \
		$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
	done

clean:	$(FRC)
	rm -f *.o $(LFILES) *.L $(AIO) $(MODSTUB)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d async

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

FRC: 
 

include $(UTSDEPEND)

include $(MAKEFILE).dep
