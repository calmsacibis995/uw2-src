#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libsl:libsl.mk	1.2"
include $(LIBRULES)

DOTSO = libsl.so
DOTA = libsl.a
DOTR = libsl.r

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT -I../../head

HFLAGS = -h /usr/lib/libsl.so

OBJS = sl_ipc.o

all:
	$(MAKE) -f libsl.mk clobber $(DOTA) PICFLAG='' CFLAGS=-g
	$(MAKE) -f libsl.mk clean $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTA): $(OBJS)
	$(AR) $(ARFLAGS) $(DOTA) `$(LORDER) $(OBJS) | $(TSORT)`

$(DOTSO): $(OBJS) libsl.funcs
	$(LD) -r -o $(DOTR) $(OBJS)
	$(CC) $(LFLAGS) $(HFLAGS) -o $(DOTSO) $(OBJS)

clean:
	rm -f *.o *.r 

clobber: clean
	rm -f $(DOTA) $(DOTSO)

install: all
	$(INS) -f $(USRLIB) -m 644 $(DOTA)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)
