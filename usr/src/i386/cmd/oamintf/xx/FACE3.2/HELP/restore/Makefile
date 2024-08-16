#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/HELP/restore/Makefile	1.3"
#ident	"$Header: $"

include $(CMDRULES)

HMENUS=$(USR)/vmsys/HELP/restore
HMENUDIRS=system
HMENUFILES=T.hdevice T.hoverwr1 T.hprivate T.hrestore T.hselect
GRP=bin
OWN=bin

all:
	@echo "nothing to build in VMSYS/HELP."

install: all
	-for d in $(HMENUDIRS); \
	do \
		[ -d $(HMENUS)/$$d ] || mkdir -p $(HMENUS)/$$d; \
		cd $$d; \
		for x in *; \
		do \
			$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(HMENUS)/$$d $$x ; \
		done; \
		cd ..; \
	done; \
	for d in $(HMENUFILES); \
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(HMENUS) $$d; \
	done;

clean:
	@echo "nothing to clean in VMSYS/HELP."

clobber: clean
	@echo "nothing to clobber in VMSYS/HELP."

size strip lintit:
