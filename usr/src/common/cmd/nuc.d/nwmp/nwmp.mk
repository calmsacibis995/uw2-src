#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nwmp:nwmp.mk	1.8"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nwmp/nwmp.mk,v 1.7 1994/12/01 19:51:44 ericw Exp $"

include $(CMDRULES)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1

#	Where MAINS are to be installed.
INSDIR = $(USRBIN)

all: nwmp

nwmp: nwmp.o
	$(CC) -o nwmp nwmp.o $(LDFLAGS) -lNwClnt -lNwCal -lNwNcp -lNwLoc -lnwutil -lthread -lnsl -lsocket -lgen

install: nwmp
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	-rm -f $(INSDIR)/nwmp
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) nwmp
	
clean:
	rm -f nwmp.o
	
clobber: clean
	rm -f nwmp

lintit:
	$(LINT) $(LINTFLAGS) nwmp.c
