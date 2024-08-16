#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)winxksh:libwin/Makefile	1.12"

include $(CMDRULES)

OBJECTS = eval.o fld.o menu.o win.o callback.o chk.o rad.o help.o keys.o wslib.o
INSDIR = $(ROOT)/$(MACH)/usr/lib/winxksh

all: allwin.o hcomp compile

win.so: $(OBJECTS) depend.a
	$(CC) -G -o win.so $(OBJECTS) -lcurses depend.a

allwin.o: $(OBJECTS)
	$(LD) -r -o $@ $(OBJECTS) -lcurses -lw

test: $(TSTOBJS)
	$(CC) -o test $(TSTOBJS) -lcurses
	
depend.a:	getsetitimer.o hrtsys.o ladd.o ldivide.o lmul.o syscall.o 
	$(AR) r depend.a getsetitimer.o hrtsys.o ladd.o ldivide.o lmul.o syscall.o 

getsetitimer.o:	$(TOOLS)/usr/ccs/lib/libc.a
	$(AR) x $(TOOLS)/usr/ccs/lib/libc.a $@

hrtsys.o:	$(TOOLS)/usr/ccs/lib/libc.a
	$(AR) x $(TOOLS)/usr/ccs/lib/libc.a $@

ladd.o:		$(TOOLS)/usr/ccs/lib/libc.a
	$(AR) x $(TOOLS)/usr/ccs/lib/libc.a $@

ldivide.o:	$(TOOLS)/usr/ccs/lib/libc.a
	$(AR) x $(TOOLS)/usr/ccs/lib/libc.a $@

lmul.o:		$(TOOLS)/usr/ccs/lib/libc.a
	$(AR) x $(TOOLS)/usr/ccs/lib/libc.a $@

syscall.o:	$(TOOLS)/usr/ccs/lib/libc.a
	$(AR) x $(TOOLS)/usr/ccs/lib/libc.a $@

$(TOOLS)/usr/ccs/lib/libc.a:

fld.o:	fld.c fld.h

help.o:	help.c

win.o:	win.c win.h

rad.o:	rad.c rad.h

chk.o:	chk.c chk.h

menu.o:	menu.c menu.h

keys.o:	keys.c keys.h win.h

wslib.o:	wslib.c wslib.h

callback.o:	callback.c callback.h

hcomp:	hcomp.o hwslib.o
	$(HCC) $(CFLAGS) $? -o $@ -lw

hcomp.o:
	$(HCC) -I/usr/include -I$(TOOLS)/usr/include -c $(CFLAGS) $?

hwslib.c:	wslib.c
	rm -f hwslib.c
	ln -s wslib.c hwslib.c

hwslib.o:	hwslib.c wslib.h
	$(HCC) -I/usr/include -I$(TOOLS)/usr/include -c $(CFLAGS) hwslib.c

compile:	compile.sh      
	@-rm -f compile
	@cp compile.sh compile; chmod 555 compile

install:
	@if [ ! -d $(INSDIR) ] ;\
	then \
		mkdir -p $(INSDIR) ;\
	fi
	@for i in scr_init winrc winxkrc compile hcomp ;\
	do \
		$(INS) -f $(INSDIR) $$i ;\
	done

clean:
	rm -f *.o hwslib.c

clobber: clean
	rm -f win.so hcomp compile
