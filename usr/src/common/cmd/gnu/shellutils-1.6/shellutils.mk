#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)gnu.cmd:shellutils-1.6/shellutils.mk	1.3"
#	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	UNIX System Laboratories, Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident "$Header: /SRCS/esmp/usr/src/nw/cmd/gnu/shellutils-1.6/shellutils.mk,v 1.3 1994/03/17 01:15:00 sharriso Exp $"

#	Makefile for GNU shell utilities

include $(CMDRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr

all: clean
	[ -f config.status ] || CC="$(CC)" LD="$(LD)" INCLUDEDIR="$(INC)" INSTALL="cp" INSTALLDATA="cp" PREFIX="$(USR)/gnu" sh configure
	$(MAKE) $(MAKEFLAGS) $@

install:
	[ -f config.status ] || CC="$(CC)" LD="$(LD)" INCLUDEDIR="$(INC)" INSTALL="cp" INSTALLDATA="cp" PREFIX="$(USR)/gnu" sh configure
	$(MAKE) $(MAKEFLAGS) $@

clean:
	[ -f config.status ] || CC="$(CC)" LD="$(LD)" INCLUDEDIR="$(INC)" INSTALL="cp" INSTALLDATA="cp" PREFIX="$(USR)/gnu" sh configure
	$(MAKE) $(MAKEFLAGS) mostlyclean

clobber:
	[ -f config.status ] || CC="$(CC)" LD="$(LD)" INCLUDEDIR="$(INC)" INSTALL="cp" INSTALLDATA="cp" PREFIX="$(USR)/gnu" sh configure
	$(MAKE) $(MAKEFLAGS) clean

lintit: mailalias.lint

mailalias.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@
