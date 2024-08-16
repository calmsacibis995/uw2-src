#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/devices/format/Makefile	1.2"
#ident "$Header: Makefile 1.1 91/08/29 $"

include $(CMDRULES)

O_SHFILES=
O_DFILES=\
	Help \
	Menu.format Menu.density \
	Form.format Text.format Text.fmtok Text.fmterr

.MUTEX:	all xinstall

all: $(O_SHFILES)

clean:

clobber: clean

lintit:

size strip: all

install: all xinstall

PROTO=Prototype
OBJECTS=$(O_SHFILES) $(O_DFILES)
include ../../Install
