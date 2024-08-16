#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/astm/astm.mk	1.2"

include $(UTSRULES)

MAKEFILE=astm.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = astm

LOCALDEF=

ASTM = astm.cf/Driver.o
LFILE = $(LINTDIR)/astm.ln

CFILES=	astm.c \
	astioctl.c

SFILES =

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	astm.ln \
	astioctl.ln

HFILES = \
	$(KBASE)/psm/ast/ast.h \
	$(KBASE)/psm/ast/ebi.h

all:	$(ASTM)

install: all
	cd astm.cf; $(IDINSTALL) -R$(CONF) -M astm

$(ASTM): astm.c astioctl.c 
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) $(LOCALDEF) -c astioctl.c; \
	UP=0; \
	for def in $(DEFLIST); do \
		case $$def in \
		-DUNIPROC) UP=1;; \
		esac; \
	done; \
	if [ $$UP -eq 1 ]; then \
		$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) $(LOCALDEF) -c astm.c; \
		$(LD) -r -o $(ASTM) astioctl.o astm.o; \
	else \
		$(LD) -r -o $(ASTM) astioctl.o; \
	fi

clean:
	-rm -f *.o $(LFILES) *.L $(ASTM) 

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e astm

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES) ; do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	$(KBASE)/psm/ast/ebi.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
