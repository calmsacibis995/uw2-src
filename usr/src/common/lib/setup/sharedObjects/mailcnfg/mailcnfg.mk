#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libsetup:sharedObjects/mailcnfg/mailcnfg.mk	1.3"

include $(LIBRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
DOTSO = mailcnfg.so
DOTR = mailcnfg.r

GLOBALINC = -I$(SGSROOT)/usr/include/nw
LOCALINC = -I../../../../../nw/head

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/setup/filetypes/mailcnfg.so

OBJS = \
	mailcnfg.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f mailcnfg.mk clean $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTSO): $(OBJS) mailcnfg.funcs
	$(LD) -r -o $(DOTR) $(OBJS)
	$(PFX)fur -l mailcnfg.funcs $(DOTR)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(OBJS)

clean:
	rm -f *.o *.r *.ln *.lerr

clobber: clean
	rm -f $(DOTSO) mailcnfg.lint

install: all
	$(INS) -f $(USRLIB)/setup/filetypes -m 755 $(DOTSO)

localinstall: $(DOTSO)
	$(INS) -f /usr/lib/setup/filetypes -m 755 $(DOTSO)

lintit: mailcnfg.lint

mailcnfg.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@
