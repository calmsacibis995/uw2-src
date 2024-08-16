#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/nw/ripx/ripx.mk	1.5"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nw/ripx/ripx.mk,v 1.2 1994/04/29 22:40:14 eric Exp $"

include $(UTSRULES)

include ../local.defs

MAKEFILE=	ripx.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/ripx

LFILE = $(LINTDIR)/ripx.ln

LFILES = \
	lddrv.ln \
	ripx_ioctls.ln \
	rip.ln \
	rrrip.ln \
	ripx_streams.ln

DRIVER = ripx.cf/Driver.o

CFILES = \
        lddrv.c \
        ripx_ioctls.c \
        rip.c \
        rrrip.c \
        ripx_streams.c

FILES = \
        lddrv.o \
        ripx_ioctls.o \
        rip.o \
        rrrip.o \
        ripx_streams.o

SRCFILES = $(CFILES)

all:  $(DRIVER)

$(DRIVER): $(FILES)
	$(LD) -r -o $@  $(FILES)

install: all
	(cd ripx.cf; $(IDINSTALL) -R$(CONF) -M ripx)

clean:
	rm -f  $(FILES) $(LFILES) *.L $(DRIVER)

clobber:	clean FRC
	-$(IDINSTALL) -R$(CONF) -d -e ripx

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


headinstall: FRC

FRC:

include $(UTSDEPEND)

include $(MAKEFILE).dep
