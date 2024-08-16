#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libmail:libmail/libmail.mk	1.8"

include $(LIBRULES)

CFLAGS = -g -DCALL_TZSET -D_EFTSAFE
.SUFFIXES: .ln
.c.ln:
	$(LINT) $(LINTFLAGS) $(DEFLIST) -c $*.c > $*.lerr
	
DOTSO = libmail.so
DOTR = libmail.r

GLOBALINC = -I$(SGSROOT)/usr/include/nw
USRLIB=$(ROOT)/$(MACH)/usr/lib

LFLAGS = -G -dy -ztext
LOCALDEF = $(PICFLAG) -DIAPX386 -Di386 -DSVR4 -DSVR4_1 -DSVr4 -DSVr4_1 -DOLDSTYLEBIND -DN_PLAT_UNIX -DN_INC_NO_OLD_MACROS -DN_USE_CRT 

HFLAGS = -h /usr/lib/libmail.so

OBJS = \
	Strerror.o \
	abspath.o \
	basename.o \
	bcollapse.o \
	cascmp.o \
	casncmp.o \
	check4mld.o \
	closefiles.o \
	compat.o \
	config.o \
	copynstr.o \
	copystream.o \
	delempty.o \
	encodefile.o \
	errexit.o \
	expargvec.o \
	getdomain.o \
	getmsgid.o \
	islocal.o \
	istext.o \
	isyesno.o \
	link.o \
	long2str.o \
	loopfork.o \
	maildir.o \
	maillock.o \
	mailsystem.o \
	mgetcharset.o \
	mgetenv.o \
	newer.o \
	notifyu.o \
	parse_ex.o \
	passfunc.o \
	popenvp.o \
	poschown.o \
	rename.o \
	rmfopen.o \
	rmopendir.o \
	s_string.o \
	setup_exec.o \
	skip2space.o \
	skipspace.o \
	sortafile.o \
	strmove.o \
	substr.o \
	systemvp.o \
	table.o \
	trimnl.o \
	xgetenv.o \
	xtmpfile.o

LINTFILES = $(OBJS:.o=.ln)

all:
	$(MAKE) -f libmail.mk clean $(DOTSO) PICFLAG=$(PICFLAG)

$(DOTSO): $(OBJS) libmail.funcs
	#$(LD) -r -o $(DOTR) $(OBJS)
	#$(PFX)fur -l libmail.funcs $(DOTR)
	$(CC) $(LFLAGS) $(HFLAGS) -o $@ $(OBJS) -lnsl

clean:
	rm -f *.o *.r *.lerr *.ln

clobber: clean
	rm -f $(DOTSO) *.lint

install: all
#	$(INS) -f $(USRINC) -m 644 -u bin -g bin maillock.h
	$(INS) -f $(USRLIB) -m 755 $(DOTSO)

localinstall: $(DOTSO)
	$(INS) -f /usr/include -m 644 -u bin -g bin maillock.h
	$(INS) -f /usr/lib -m 755 $(DOTSO)


getdomain.o: libmail.h stdc.h s_string.h config.h
mgetenv.o: mail.h

lintit: libmail.lint

libmail.lint: $(LINTFILES)
	$(LINT) $(LINTFLAGS) $(LINTFILES) > $@
