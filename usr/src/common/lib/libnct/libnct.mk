#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnct:libnct.mk	1.3"
include $(LIBRULES)

DOTSO = libnct.so
DOTA = libnct.a
DOTR = libnct.r

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/libnct.so

OBJS = conn.o util.o error.o

all:
	$(MAKE) -f libnct.mk clobber $(DOTA) PICFLAG='' CFLAGS=-g
	$(MAKE) -f libnct.mk clean $(DOTSO) PICFLAG=$(PICFLAG)
#	$(MAKE) -f libnct.mk $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTA): $(OBJS)
	$(AR) $(ARFLAGS) $(DOTA) `$(LORDER) $(OBJS) | $(TSORT)`

$(DOTSO): $(OBJS) libnct.funcs
	$(LD) -r -o $(DOTR) $(OBJS)
	$(PFX)fur -l libnct.funcs $(DOTR)
	$(CC) $(LFLAGS) $(HFLAGS) -o $(DOTSO) $(OBJS)

clean:
	rm -f *.o *.r 

clobber: clean
	rm -f $(DOTA) $(DOTSO)

install: all
	$(INS) -f $(USRLIB) -m 644 $(DOTA)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)
