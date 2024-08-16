#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libmail:sharedObjects/passwd/passwd.mk	1.7"

include $(LIBRULES)

.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
DOTSO = passwd.so
DOTR = passwd.r

GLOBALINC = -I$(SGSROOT)/usr/include/nw
USRLIB=$(ROOT)/$(MACH)/usr/lib

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/mail/libalias/passwd.so

OBJS = \
	funcs.o \
	passwd.o \
	passfunc.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f passwd.mk clean $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTSO): $(OBJS) passwd.funcs
	$(LD) -r -o $(DOTR) $(OBJS)
	$(PFX)fur -l passwd.funcs $(DOTR)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(OBJS)

clean:
	rm -f *.o *.r *.ln *.lerr

clobber: clean
	rm -f $(DOTSO) passwd.lint

install: all
	$(INS) -f $(USRLIB)/mail/libalias -m 755 $(DOTSO)

localinstall: $(DOTSO)
	$(INS) -f /usr/lib/mail/libalias -m 755 $(DOTSO)

lintit: passwd.lint

passwd.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@
