#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/nw/ipx/ipxr/ipxr.mk	1.3"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Id: ipxr.mk,v 1.2 1994/02/18 15:17:21 vtag Exp $"

include $(UTSRULES)

include ../../local.defs

MAKEFILE=	ipxr.mk
KBASE = ../../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/ipx/ipxr

IPX = ../ipx.cf/Driver.o
LFILE = $(LINTDIR)/ipx.ln

CFILES = ipxr_streams.c
FILES = ipxr_streams.o
LFILES = ipxr_streams.ln

SRCFILES = $(CFILES)

LIPMX_COMMON = $(KBASE)/net/nw/ipx/lipmx
DRV_LOAD = $(KBASE)/net/nw/ipx/drvload

OBJS = \
        $(DRV_LOAD)/lddrv.o

LIPMX_COMMON_OBJS = \
        $(LIPMX_COMMON)/lipmx_streams.o \
        $(LIPMX_COMMON)/lipmx_ioctls.o \
        $(LIPMX_COMMON)/lipmx.o \
        $(LIPMX_COMMON)/norouter.o \
        $(LIPMX_COMMON)/rripx.o


all: $(IPX)

install: all

$(IPX): $(FILES) $(LIPMX_COMMON_OBJS) $(OBJS)
	$(LD) -r -o $(IPX) $(LIPMX_COMMON_OBJS) $(FILES) $(OBJS)

$(LIPMX_COMMON_OBJS):
	(cd $(LIPMX_COMMON); make -f *.mk)

$(OBJS):
	(cd $(DRV_LOAD); make -f *.mk)

clean:
	-rm -f *.o $(LFILES) *.L $(IPX)
	-(cd $(LIPMX_COMMON); make -f *.mk clean)
	-(cd $(DRV_LOAD); make -f *.mk clean)

clobber: clean

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE):	$(LINTDIR) $(LFILES)
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

headinstall:


include $(UTSDEPEND)

include $(MAKEFILE).dep
