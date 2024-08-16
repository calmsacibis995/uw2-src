#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/Makefile	1.3"
#ident	"$Header: $"

include $(CMDRULES)

DIRS=FACE3.2 OAM4.0

all install clean clobber size strip lintit:
	@echo "====== XX DIRECTORY"
	-for dir in $(DIRS) ; \
	do\
		( cd $$dir ; $(MAKE) $(MAKEARGS) $(@) ) ; \
	done
