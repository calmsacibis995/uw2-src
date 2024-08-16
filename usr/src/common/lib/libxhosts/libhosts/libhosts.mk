#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libhosts:libhosts/libhosts.mk	1.2"

include $(LIBRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
DOTSO = libhosts.so
DOTR = libhosts.r
INCMAIL = $(ROOT)/$(MACH)/usr/include/mail

LOCALINC = -I../../../head

LFLAGS = -G -dy -ztext -L/usr/lib
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/libhosts.so

OBJS = \
	hosts.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f libhosts.mk clean $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTSO): $(OBJS) libhosts.funcs ../../libtree/libtree.so
	$(LD) -r -o $(DOTR) $(OBJS)
	$(PFX)fur -l libhosts.funcs $(DOTR)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(OBJS) ../../libtree/libtree.so -ldl

../../libtree/libtree.so:
	cd ../../libtree; make -f *.mk $(@f)

clean:
	rm -f *.o *.r *.ln *.lerr

clobber: clean
	rm -f $(DOTSO) libhosts.lint

install: all $(INCMAIL)
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)
	$(INS) -f $(INCMAIL) -m 755 hosts.h

$(INCMAIL):
	mkdir -p $(INCMAIL)

localinstall: $(DOTSO)
	$(INS) -f /usr/lib -m 755 $(DOTSO)
	$(INS) -f /usr/include/mail -m 755 hosts.h

lintit: libhosts.lint

libhosts.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@

hosts.ln hosts.o: ../../../head/mail/tree.h
