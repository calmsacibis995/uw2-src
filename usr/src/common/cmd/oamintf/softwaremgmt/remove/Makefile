#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/softwaremgmt/remove/Makefile	1.2.6.3"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

O_SHFILES=
O_DFILES=\
	Form.remove
HELPFILES=\
	Help
HELPDEST=\
	remove/Help

.MUTEX:	 all xinstall

all: $(O_SHFILES)

clobber clean:
	:

lintit:
	:

size strip: all
	:

install: all xinstall

PROTO=../Prototype
OBJECTS=$(HELPFILES) $(O_SHFILES) $(O_DFILES)
OBJDEST=$(HELPDEST)
include ../../Install
