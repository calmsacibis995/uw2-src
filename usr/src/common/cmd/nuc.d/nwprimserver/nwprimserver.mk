#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nwprimserver:nwprimserver.mk	1.1"
include $(CMDRULES)

INSDIR = $(USRBIN)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT

DEV1 = -I../../../head
#CFLAGS = -g
#LDFLAGS = -g

all: nwprimserver

nwprimserver: nwprimserver.o
	$(CC) -o nwprimserver nwprimserver.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc $(TOOLS)/usr/lib/novell.so -lnwutil \
	 -lthread -lgen -lnsl -lnct

install: nwprimserver
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/nwprimserver
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) nwprimserver
	
clean:
	rm -f nwprimserver.o
	
clobber: clean
	rm -f nwprimserver

lintit:
	$(LINT) $(LINTFLAGS) nwprimserver.c
