#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/HELP/logins/Makefile	1.3"
#ident	"$Header: $"

include $(CMDRULES)

HMENUS=$(ROOT)/$(MACH)/usr/vmsys/HELP/logins
HMENUDIRS=
HMENUFILES=T.hlogins
GRP=bin
OWN=bin

all:
	@echo "nothing to build in VMSYS/HELP/logins."

install: all
	[ -d $(HMENUS) ] || mkdir -p $(HMENUS)
	-for f in $(HMENUFILES) ; \
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(HMENUS) $$f ; \
	done

clean:
	@echo "nothing to clean in VMSYS/HELP/logins."

clobber: clean
	@echo "nothing to clobber in VMSYS/HELP/logins."

size strip lintit:
