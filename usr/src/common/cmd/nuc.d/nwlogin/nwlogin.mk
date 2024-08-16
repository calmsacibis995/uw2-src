#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nwlogin:nwlogin.mk	1.7"
include $(CMDRULES)

INSDIR = $(USRBIN)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT

DEV1 = -I../../../head

all: nwlogin

nwlogin: nwlogin.o
	$(CC) -o nwlogin nwlogin.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc -lnwutil -l thread $(TOOLS)/usr/lib/novell.so -lgen \
         -lnsl -lnct

install: nwlogin
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/nwlogin
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) nwlogin
	
clean:
	rm -f nwlogin.o
	
clobber: clean
	rm -f nwlogin

lintit:
	$(LINT) $(LINTFLAGS) nwlogin.c
