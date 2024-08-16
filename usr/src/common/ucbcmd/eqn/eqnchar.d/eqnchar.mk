#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ucb:common/ucbcmd/eqn/eqnchar.d/eqnchar.mk	1.2"
#ident	"$Header: $"
#     Portions Copyright(c) 1988, Sun Microsystems, Inc.
#     All Rights Reserved


include $(CMDRULES)

INSDIR= $(USR)/ucblib

OWN = bin

GRP = bin

# Files which live in the current directory are copied to the destination.
#
FILES=	eqnchar 

all:	${FILES}

install:
	[ -d $(INSDIR)/pub ] || mkdir -p $(INSDIR)/pub
	$(CH)-chmod 755 $(INSDIR)/pub
	$(CH)-chgrp $(GRP) $(INSDIR)/pub
	$(CH)-chown $(OWN) $(INSDIR)/pub
	$(INS) -f $(INSDIR)/pub -u $(OWN) -g $(GRP) -m 644 $(FILES)

clean:
