#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/nw/ipx/ipxs/ipxs.mk	1.3"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/net/nw/ipx/ipxs/ipxs.mk,v 1.1 1994/01/28 17:50:36 vtag Exp $"

include $(UTSRULES)

include ../../local.defs

MAKEFILE=	ipxs.mk
KBASE = ../../../..
LINTDIR = $(KBASE)/lintdir
DIR = net/nw/ipx/ipxs

sysHeader = ipx_tune.h

IPX = ../ipx.cf/Driver.o
LFILE = $(LINTDIR)/ipx.ln

CFILES = ipx.c
FILES = ipx.o
LFILES = ipx.ln

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

headinstall: localhead FRC

localhead:	$(sysHeader)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $(sysHeader)

FRC:

include $(UTSDEPEND)

include $(MAKEFILE).dep
