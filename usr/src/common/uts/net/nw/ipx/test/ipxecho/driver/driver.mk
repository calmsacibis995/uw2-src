#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ipx-test:common/uts/net/nw/ipx/test/ipxecho/driver/driver.mk	1.2"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id"

include $(UTSRULES)

include ../../../../local.defs

MAKEFILE=	driver.mk
KBASE = ../../../../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/ipx/test/ipxecho/driver


LFILE = $(LINTDIR)/ipxecho.ln

LFILES = \
        lddrv.ln \
        ipxecho.ln


DRIVER = ipxecho.cf/Driver.o

CFILES = \
        lddrv.c \
        ipxecho.c


FILES = \
        lddrv.o \
        ipxecho.o

SRCFILES = $(CFILES)

all: $(DRIVER)

$(DRIVER): $(FILES)
	$(LD) -r -o $@  $(FILES)

install: all
	(cd ipxecho.cf; $(IDINSTALL) -R$(CONF) -M ipxecho)

clean:
	rm -f  $(FILES) $(LFILES) *.L $(DRIVER)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e ipxecho

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE):       $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done


include $(UTSDEPEND)

include $(MAKEFILE).dep
