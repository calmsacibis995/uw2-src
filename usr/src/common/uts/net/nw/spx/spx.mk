#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/nw/spx/spx.mk	1.6"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id"

include $(UTSRULES)

include ../local.defs

MAKEFILE=	spx.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/spx

sysHeader = spx_tune.h

LFILE = $(LINTDIR)/spx.ln

LFILES = \
        lddrv.ln \
        spx2.ln \
        spx2rd.ln \
        spx2wr.ln


DRIVER = spx.cf/Driver.o

CFILES = \
        lddrv.c \
        spx2.c \
        spx2rd.c \
        spx2wr.c


FILES = \
        lddrv.o \
        spx2.o \
        spx2rd.o \
        spx2wr.o

SRCFILES = $(CFILES)

all:  $(DRIVER)

$(DRIVER): $(FILES)
	$(LD) -r -o $@  $(FILES)

install: all
	(cd spx.cf; $(IDINSTALL) -R$(CONF) -M nspx)

clean:
	rm -f  $(FILES) $(LFILES) *.L $(DRIVER)

clobber:	clean FRC
	-$(IDINSTALL) -R$(CONF) -d -e nspx

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE):       $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done


fnames:
	@for i in $(SRCFILES); do \
                echo $$i; \
        done


headinstall: localhead FRC

localhead:	$(sysHeader)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $(sysHeader)

FRC:

include $(UTSDEPEND)

include $(MAKEFILE).dep
