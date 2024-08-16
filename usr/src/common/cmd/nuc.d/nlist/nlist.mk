#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nlist:nlist.mk	1.3"
include $(CMDRULES)

INSDIR = $(USRBIN)

LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT

DEV1 = -I../../../head
#CFLAGS = -g
#LDFLAGS = -g

all: nlist

nlist: nlist.o list.o bindery.o
	$(CC) -o nlist nlist.o list.o bindery.o $(LDFLAGS) \
         -lNwCal -lNwNcp -lNwClnt -lNwLoc $(TOOLS)/usr/lib/novell.so -lnwutil -lthread -lgen \
         -lnsl -lnct

install: nlist
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	rm -f $(INSDIR)/nlist
	$(INS) -f $(INSDIR) -m 04550 -u $(OWN) -g $(GRP) nlist
	
clean:
	rm -f nlist.o list.o bindery.o
	
clobber: clean
	rm -f nlist

lintit:
	$(LINT) $(LINTFLAGS) nlist.c list.c bindery.c
