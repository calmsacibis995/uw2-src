#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nwcloseconns:nwcloseconns.mk	1.8"

include $(CMDRULES)

INSDIR = $(USRSBIN)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT

GLOBALINC = -I$(SGSROOT)/usr/include/nw
LOCALINC = -I../../../lib/libnwClnt/headers 

LOCALLIBS = -lNwClnt -lNwCal -lNwNcp -lNwLoc -lnwutil -lthread -lnsl -lgen

nwcloseconns: nwcloseconns.o
	$(CC) -o nwcloseconns nwcloseconns.o $(GLOBALINC) \
		$(LOCALINC) $(LOCALLIBS)

all: nwcloseconns 

clean:
	rm -f *.o  

clobber: clean
	rm -f nwcloseconns

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/nwcloseconns
	$(INS) -f $(INSDIR) -m 550 -u $(OWN) -g $(GRP) nwcloseconns

lintit:
	$(LINT) $(LINTFLAGS) nwcloseconns.c

