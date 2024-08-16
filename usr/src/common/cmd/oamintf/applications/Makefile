#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/applications/Makefile	1.6.4.4"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

.MUTEX: all xinstall

HELPFILES=Help 
HELPDEST=applmgmt/Help 

O_DFILES=\
	appl.menu

.MUTEX: all xinstall

all:

clean: 

clobber: clean

lintit:

size strip: all

install: all xinstall

PROTO=../Prototype
OBJECTS=$(HELPFILES) $(O_DFILES)
OBJDEST=$(HELPDEST)
include ../Install
