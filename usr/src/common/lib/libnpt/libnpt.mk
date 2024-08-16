#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnpt:libnpt.mk	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnpt/libnpt.mk,v 1.4 1994/06/10 20:51:12 wrees Exp $"

include $(LIBRULES)

DOTSO = libnpt.so
DOTA = libnpt.a
DOTR = libnpt.r

GLOBALINC = -I$(SGSROOT)/usr/include/nw
LOCALINC = -I. -I../../head/inc

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/libnpt.so

OBJS = \
		abort.o \
		addnotfy.o \
		addqueue.o \
		attachfs.o \
		attachps.o \
		cancel.o \
		chgnotfy.o \
		chgqueue.o \
		delnotfy.o \
		delqueue.o \
		detachfs.o \
		detachps.o \
		down.o \
		eject.o \
		error.o \
		getnotfy.o \
		getprint.o \
		getqueue.o \
		getremot.o \
		getserve.o \
		jobstat.o \
		loginps.o \
		mark.o \
		mount.o \
		preferrd.o \
		psinfo.o \
		pstatus.o \
		rewind.o \
		setmode.o \
		setremot.o \
		spx.o \
		start.o \
		stop.o \
		useremot.o

all:
	$(MAKE) -f libnpt.mk clobber $(DOTA) PICFLAG='' CFLAGS=
	$(MAKE) -f libnpt.mk clean $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTA): $(OBJS)
	$(AR) $(ARFLAGS) $(DOTA) `$(LORDER) $(OBJS) | $(TSORT)`

$(DOTSO): $(OBJS) 
	$(LD) -r -o $(DOTR) $(OBJS)
	$(CC) $(LFLAGS) $(HFLAGS) -o $(DOTSO) $(OBJS)

clean:
	rm -f *.o *.r 

clobber: clean
	rm -f $(DOTA) $(DOTSO)

install: all
	$(INS) -f $(USRLIB) -m 644 $(DOTA)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)
