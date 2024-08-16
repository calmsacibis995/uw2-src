#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libserver:libserver.mk	1.12"

include $(LIBRULES)
USRXLIB		=	$(ROOT)/$(MACH)/usr/X/lib

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
DOTSO = libserver.so

DOTSOR = libRserver.so
DOTSOX = libXserver.so
DOTR1 = libserver.r
DOTR2 = libXserver.r
MAKEFILE=libserver.mk

GLOBALINC = -I$(SGSROOT)/usr/include/nw
LOCALINC = -I../../../nw/head
XINC=$(ROOT)/usr/src/$(WORK)/X11R5

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS1 = -h /usr/lib/libserver.so
HFLAGS2 = -h /usr/X/lib/libserver.so

OBJS = \
	link.o	\
	list.o	\
	server.o \
	table.o

XOBJS = \
	link.o	\
	list.o \
	Xserver.o \
	table.o

LINTERRFILES = $(OBJS:.o=.lerr)
XLINTERRFILES = $(XOBJS:.o=.lerr)
LINTFILES = $(OBJS:.o=.ln)
XLINTFILES = $(XOBJS:.o=.ln)

all:
	$(MAKE) -f $(MAKEFILE) $(DOTSOR) PICFLAG=$(PICFLAG)

gui:	$(DOTSOX)
	$(MAKE) -f $(MAKEFILE) clean_gui $(DOTSOX) PICFLAG=$(PICFLAG)

reg:	$(DOTSOR)
	$(MAKE) -f $(MAKEFILE) clean_reg $(DOTSOR) PICFLAG=$(PICFLAG)

$(DOTSOR): $(OBJS) libserver.funcs
	$(LD) -r -o $(DOTR1) $(OBJS)
	#$(PFX)fur -l libserver.funcs $(DOTR1)
	$(CC) $(LFLAGS) $(HFLAGS1) -o $@ $(OBJS)

$(DOTSOX): $(XOBJS) libXserver.funcs
	$(LD) -r -o $(DOTR2) $(XOBJS)
	#$(PFX)fur -l libXserver.funcs $(DOTR2)
	$(CC) $(LFLAGS) $(HFLAGS2) -o $@ $(XOBJS)

Xserver.o: server.c
	$(CC) $(CFLAGS) $(DEFLIST) -I$(XINC) -DX_SERVER -c server.c
	mv server.o Xserver.o

Xserver.ln: server.c
	$(LINT) $(LINTFLAGS) $(DEFLIST) -I$(XINC) -DX_SERVER -c $? > Xserver.lerr

$(USRXLIB):
	mkdir -p $(USRXLIB)

clean:
	rm -f *.o *.r *.ln *.lerr

clean_reg:
	rm -f $(OBJS) libRserver.r $(LINTFILES) $(LINTERRFILES)

clean_gui:
	rm -f $(XOBJS) libXserver.r $(XLINTFILES) $(XLINTERRFILES)

clobber: clean
	rm -f $(DOTSOR) $(DOTSOX) libserver.lint libXserver.lint

install: all
	rm -f $(DOTSO)
	- ln libRserver.so $(DOTSO)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)
	rm -f $(DOTSO)

install_reg: reg
	rm -f $(DOTSO)
	- ln $(DOTSOR) $(DOTSO)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)
	rm -f $(DOTSO)

install_gui: gui $(USRXLIB)
	rm -f $(DOTSO)
	- ln $(DOTSOX) $(DOTSO)
	$(INS) -f $(USRXLIB) -m 755 $(DOTSO)
	rm -f $(DOTSO)

localinstall: libRserver.so
#libXserver.so
	rm -f libserver.so
	- ln libRserver.so libserver.so
	$(INS) -f /usr/lib -m 755 libserver.so
	rm -f libserver.so
	#- ln libXserver.so libserver.so
	#$(INS) -f /usr/X/lib -m 755 libserver.so
	#rm -f libserver.so

lintit: libserver.lint libXserver.lint

libserver.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

libXserver.lint: $(XLINTFILES)
	$(LINT) $(XLINTFLAGS) $(LINTFILES) > $@
