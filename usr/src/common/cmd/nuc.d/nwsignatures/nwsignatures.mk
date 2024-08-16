#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nwsignatures:nwsignatures.mk	1.9"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nwsignatures/nwsignatures.mk,v 1.7.2.1 1995/02/12 23:42:16 hashem Exp $"

include $(CMDRULES)

#	Where MAINS are to be installed.
INSDIR = $(USRSBIN)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT


all: nwsignatures

nwsignatures: nwsignatures.o
	$(CC) -o nwsignatures nwsignatures.o $(LDFLAGS) -lNwClnt -lNwCal -lNwNcp -lNwLoc -lnwutil -lthread -lnsl -lgen -lnct

install: nwsignatures
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/nwsignatures
	$(INS) -f $(INSDIR) -m 0550 -u $(OWN) -g $(GRP) nwsignatures
	
clean:
	rm -f nwsignatures.o
	
clobber: clean
	rm -f nwsignatures

lintit:
	$(LINT) $(LINTFLAGS) nwsignatures.c
