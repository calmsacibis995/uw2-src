#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)dfs.cmds:unshareall/unshareall.mk	1.3.5.3"
#ident "$Header: unshareall.mk 1.2 91/04/05 $"

include $(CMDRULES)

INSDIR = $(USRSBIN)
OWN = bin
GRP = bin
FRC =

all: unshareall

unshareall: unshareall.sh 
	cp unshareall.sh unshareall
	chmod 555 unshareall

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m 0555 -u $(OWN) -g $(GRP) unshareall

clean:

clobber: clean
	rm -f unshareall
FRC:
