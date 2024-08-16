#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libtree:libtree.mk	1.2"

include $(LIBRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
DOTSO = libtree.so
DOTR = libtree.r

LOCALINC = -I../../head
USRLIB=$(ROOT)/$(MACH)/usr/lib

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/libtree.so

OBJS = \
	tree.o \
	link.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f libtree.mk clean $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTSO): $(OBJS) libtree.funcs
	$(LD) -r -o $(DOTR) $(OBJS)
	$(PFX)fur -l libtree.funcs $(DOTR)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(OBJS) -ldl

clean:
	rm -f *.o *.r *.ln *.lerr

clobber: clean
	rm -f $(DOTSO) libtree.lint

install: all
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)

localinstall: $(DOTSO)
	$(INS) -f /usr/lib -m 755 $(DOTSO)

lintit: libtree.lint

libtree.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

tree.ln tree.o: ../../head/mail/tree.h
