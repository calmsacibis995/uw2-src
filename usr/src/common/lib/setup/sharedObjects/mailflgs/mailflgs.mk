#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libsetup:sharedObjects/mailflgs/mailflgs.mk	1.2"

include $(LIBRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
DOTSO = mailflgs.so
DOTR = mailflgs.r

GLOBALINC = -I$(SGSROOT)/usr/include/nw
LOCALINC = -I../../../../../nw/head

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/setup/filetypes/mailflgs.so

OBJS = \
	mailflgs.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f mailflgs.mk clean $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTSO): $(OBJS) mailflgs.funcs
	$(LD) -r -o $(DOTR) $(OBJS)
	$(PFX)fur -l mailflgs.funcs $(DOTR)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(OBJS)

clean:
	rm -f *.o *.r *.ln *.lerr

clobber: clean
	rm -f $(DOTSO) mailflgs.lint

install: all
	$(INS) -f $(USRLIB)/setup/filetypes -m 755 $(DOTSO)

localinstall: $(DOTSO)
	$(INS) -f /usr/lib/setup/filetypes -m 755 $(DOTSO)

lintit: mailflgs.lint

mailflgs.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@
