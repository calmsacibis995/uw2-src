#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libsetup:sharedObjects/smfcnfg/smfcnfg.mk	1.3"

include $(LIBRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr

DOTSO = smfcnfg.so
DOTR = smfcnfg.r

GLOBALINC = -I$(SGSROOT)/usr/include/nw
LOCALINC = -I../../../../../nw/head

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/setup/filetypes/smfcnfg.so

OBJS = \
	passwd.o \
	smfcnfg.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f smfcnfg.mk $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTSO): $(OBJS) smfcnfg.funcs
	$(LD) -r -o $(DOTR) $(OBJS)
	$(PFX)fur -l smfcnfg.funcs $(DOTR)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(OBJS) -lsetup

clean:
	rm -f *.o *.r *.ln *.lerr

clobber: clean
	rm -f $(DOTSO) smfcnfg.lint

install: all
	$(INS) -f $(USRLIB)/setup/filetypes -m 755 $(DOTSO)

localinstall: $(DOTSO)
	$(INS) -f /usr/lib/setup/filetypes -m 755 $(DOTSO)

lintit: smfcnfg.lint

smfcnfg.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@
