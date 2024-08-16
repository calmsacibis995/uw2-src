#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)drf:drf.mk	1.1"
#

include $(CMDRULES)

DIRS =	cmd instcmd hcomp prt_files 

all .DEFAULT:
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		$(MAKE) -f $$d.mk $@;\
		cd .. ;\
	done;

install: all
	@set -e;\
	for d in $(DIRS);\
	do\
		cd $$d;\
		$(MAKE) -f $$d.mk $@;\
		cd ..;\
	done;
