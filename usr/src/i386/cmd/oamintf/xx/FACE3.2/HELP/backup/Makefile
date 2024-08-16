#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:i386/cmd/oamintf/xx/FACE3.2/HELP/backup/Makefile	1.3"
#ident	"$Header: $"

include $(CMDRULES)

HMENUS=$(USR)/vmsys/HELP/backup
HMENUDIRS=private sched system
HMENUFILES=T.hbackup
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
	$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(HMENUS) $(HMENUFILES)

clean:
	@echo "nothing to clean in VMSYS/HELP."

clobber: clean
	@echo "nothing to clobber in VMSYS/HELP."

size strip lintit:
