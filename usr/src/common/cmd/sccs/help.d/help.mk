#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)sccs:help.d/help.mk	6.11"
#
#

include $(CMDRULES)

HELPLIB = $(CCSLIB)/help

FILES1 = ad bd cb cm cmds co de default
FILES2 = ge he prs rc un ut

all:

install: all
	[ -d $(HELPLIB) ] || mkdir -p $(HELPLIB)
	$(CH)-chmod 775 $(HELPLIB)
	-cd $(HELPLIB); rm -f $(FILES2) $(FILES2)
	cp $(FILES1) $(FILES2) $(HELPLIB)
	-cd $(HELPLIB); $(CH)chmod 664 $(FILES1)	$(FILES2)
	-@cd $(HELPLIB); $(CH)chgrp $(GRP) $(FILES1) $(FILES2) .
	-@cd $(HELPLIB); $(CH)chown $(OWN) $(FILES1) $(FILES2) .

clean:

clobber:
