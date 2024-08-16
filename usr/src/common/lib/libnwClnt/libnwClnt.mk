#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libNwClnt:libnwClnt.mk	1.11"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/libnwClnt.mk,v 1.12 1994/09/26 17:18:44 rebekah Exp $"

include $(LIBRULES)

.SUFFIXES: .P

DOTSO = libNwClnt.so
DOTA = libNwClnt.a

GLOBALINC =  -I$(INC) 
LOCALINC = -I./headers -I../../head -I../../head/nw -I../../head/inc

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT -D_REENTRANT

HFLAGS = -h /usr/lib/libNwClnt.so

OBJS_O = \
	nwClnt.o \
	conneng.o \
	dsreq.o \
	nwmpio.o \
	nwmpserv.o \
	nwmptask.o \
	nwclient.o \
	spil.o \
	requtils.o \
	requester.o

OBJS_P=$(OBJS_O:.o=.P)

.c.P:
	$(CC) -Wa,-o,$*.P $(CFLAGS) $(DEFLIST) $(PICFLAG) -c $*.c

all: dota dotso

dota: $(OBJS_O)
	$(AR) $(ARFLAGS) $(DOTA) $(OBJS_O) 

dotso: $(OBJS_P)
	$(LD) $(LFLAGS) $(HFLAGS) -o $(DOTSO) $(OBJS_P) -lnwutil

clean:
	rm -f *.o *.P

clobber: clean
	rm -f $(DOTA) $(DOTSO)

install: all
	$(INS) -f $(USRLIB) -m 644 $(DOTA)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)
	$(INS) -f $(ROOT)/$(MACH)/usr/include -m 444 headers/nucinit.h

