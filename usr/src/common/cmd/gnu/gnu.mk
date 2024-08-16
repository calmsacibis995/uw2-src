#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)gnu.cmd:gnu.mk	1.4"
#ident "$Header: /SRCS/esmp/usr/src/nw/cmd/gnu/gnu.mk,v 1.6 1994/03/31 16:34:07 eric Exp $"

#	Makefile for GNU distribution

include $(CMDRULES)

GNUBIN = $(ROOT)/$(MACH)/usr/gnu/bin
GNULIB = $(ROOT)/$(MACH)/usr/gnu/lib
GNULIBPERL = $(ROOT)/$(MACH)/usr/gnu/lib/perl

all clean clobber lintit localinstall:
	cd shellutils-1.6; $(MAKE) $(MAKEFLAGS) -f shellutils.mk $@
	cd perl-4.036; $(MAKE) $(MAKEFLAGS) -f perl.mk $@

install: $(GNUBIN) $(GNULIB) $(GNULIBPERL)
	cd shellutils-1.6; $(MAKE) $(MAKEFLAGS) -f shellutils.mk $@
	cd perl-4.036; $(MAKE) $(MAKEFLAGS) -f perl.mk $@

$(GNULIBPERL) $(GNUBIN) $(GNULIB):
	mkdir -p $@
