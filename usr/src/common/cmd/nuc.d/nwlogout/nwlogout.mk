#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nwlogout:nwlogout.mk	1.6"
include $(CMDRULES)

INSDIR = $(USRBIN)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT

DEV1 = -I../../../head

all: nwlogout

nwlogout: nwlogout.o
	$(CC) -o nwlogout nwlogout.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc -lnwutil -lthread $(TOOLS)/usr/lib/novell.so -lgen \
         -lnsl -lnct

install: nwlogout
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/nwlogout
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) nwlogout
	
clean:
	rm -f nwlogout.o
	
clobber: clean
	rm -f nwlogout

lintit:
	$(LINT) $(LINTFLAGS) nwlogout.c
