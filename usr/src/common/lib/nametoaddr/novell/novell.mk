#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)nametoaddr:common/lib/nametoaddr/novell/novell.mk	1.15"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/nametoaddr/novell/novell.mk,v 1.13 1994/07/14 21:36:22 eric Exp $"

include $(LIBRULES)

LOCALLFLGS = -G -dy -ztext

LOCALDEF = $(PICFLAG) -DN_PLAT_UNIX -D_REENTRANT

OBJS = \
	novell.o \
	file_db.o \
	novell_mt.o

SRCS = \
	novell.c \
	file_db.c \
	novell_mt.c

.MUTEX:	novell.so novell_nwnet.so

all:	novell.so novell_nwnet.so

novell.so: $(SRCS)
	$(MAKE) -f novell.mk LOCALDEF="$(LOCALDEF) -DCHECK_BINDERY" clean t1

novell_nwnet.so: $(SRCS)
	$(MAKE) -f novell.mk LOCALDEF="$(LOCALDEF)" clean t2

t1:	$(OBJS)
	$(LD) $(LOCALLFLGS) -h /usr/lib/novell.so -o novell.so \
	$(OBJS) -lnwutil -lNwClnt -lNwCal -lNwNcp -lNwLoc -lnsl

t2:	$(OBJS)
	$(LD) $(LOCALLFLGS) -h /usr/lib/novell_nwnet.so -o novell_nwnet.so \
	$(OBJS) -lnwutil -lnsl

clean:
	rm -f *.o

clobber: clean
	rm -f novell.so novell_nwnet.so

install: all
	$(INS) -f $(USRLIB) -m 755 novell.so
	$(INS) -f $(USRLIB) -m 755 novell_nwnet.so
	[ -d $(ETC)/netware ] || mkdir -p $(ETC)/netware
	$(INS) -f $(ETC)/netware -m 444 saptypes
