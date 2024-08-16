#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386sym:io/dlpi_ether/dlpi_ether.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	dlpi_ether.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/dlpi_ether

LOCALDEF = -DEGL -DALLOW_SET_EADDR

EGL = egl.cf/Driver.o
LFILE = $(LINTDIR)/egl.ln

FILES = \
	egl.o \
	dlpi_egl.o

SRCFILES = \
	egl.c \
	dlpi_ether.c

LFILES = \
	egl.ln \
	dlpi_ether.ln


all: $(EGL)

install: all
	(cd egl.cf; $(IDINSTALL) -R$(CONF) -M egl)

dlpi_egl.o : dlpi_ether.h dlpi_ether.c dlpi_egl.h
	$(CC)  $(CFLAGS) $(INCLIST) $(DEFLIST) -c dlpi_ether.c && mv dlpi_ether.o dlpi_egl.o

egl.o : egl.c dlpi_ether.h

$(EGL): $(FILES)
	$(LD) -r -o $(EGL) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e egl

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
	dlpi_ether.h
confHeaders = \
	conf_attr.h

headinstall: $(sysHeaders) $(confHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@-[ -d $(INC)/conf/dlpi_ether ] || mkdir -p $(INC)/conf/dlpi_ether
	@for f in $(confHeaders); \
	 do \
	    $(INS) -f $(INC)/conf/dlpi_ether -m $(INCMODE) \
						-u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
