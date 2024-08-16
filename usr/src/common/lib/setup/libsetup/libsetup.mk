#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libsetup:libsetup/libsetup.mk	1.6"

include $(LIBRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
DOTSO = libsetup.so
DOTR = libsetup.r

GLOBALINC = -I$(SGSROOT)/usr/include/nw
LOCALINC = -I../../../../nw/head
USRINC=$(ROOT)/$(MACH)/usr/include

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/libsetup.so

OBJS = \
	basename.o \
	link.o \
	setupFile.o \
	setupVar.o \
	setupWeb.o \
	setupMove.o \
	table.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f libsetup.mk $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTSO): $(OBJS) libsetup.funcs
	$(LD) -r -o $(DOTR) $(OBJS)
	$(PFX)fur -l libsetup.funcs $(DOTR)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(OBJS) -ldl

clean:
	rm -f *.o *.r *.ln *.lerr

clobber: clean
	rm -f $(DOTSO) libsetup.lint

install: all $(USRINC)/mail
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)
	$(INS) -f $(USRINC)/mail -m 444 setupFile.h
	$(INS) -f $(USRINC)/mail -m 444 setupTypes.h
	$(INS) -f $(USRINC)/mail -m 444 setupWeb.h
	$(INS) -f $(USRINC)/mail -m 444 setupVar.h
	$(INS) -f $(USRINC)/mail -m 444 setupMove.h

$(USRINC)/mail:
	mkdir -p $@

localinstall: $(DOTSO)
	$(INS) -f /usr/lib -m 755 $(DOTSO)

lintit: libsetup.lint

libsetup.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@
