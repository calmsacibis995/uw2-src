#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)rtld:i386/makefile	1.9.2.12"

# makefile for i386 version of the run-time linker

include $(LIBRULES)

SGSBASE = ../../../../cmd/sgs
SGSINC=$(SGSBASE)/inc
MODE= -DPIC -KPIC -Kno_host
CPUDIR = ../$(CPU)
GLOBALINC =
LIBCMACH = ../../$(CPU)
LIBCMACHINC = ../../$(CPU)/inc
LIBCPORTINC = ../../port/inc
LOCALDEF = -D$(CPU) -DWEITEK 
LOCALINC = -I$(CPUDIR) -I$(LIBCMACHINC) -I$(LIBCPORTINC) -I$(SGSINC)/common -I$(SGSINC)/$(CPU) -I$(INC)
VERSDEF=$(LIBCMACH)/ansi.def
M4DEFS=$(LIBCMACH)/m4.def $(LIBCMACH)/sys.def $(VERSDEF)
PROPTS= -dxerox -omode=l -omode=h
AWK=awk
ECHO=echo
HFILES= $(CPUDIR)/rtld.h $(CPUDIR)/dllib.h $(CPUDIR)/externs.h \
	$(CPUDIR)/machdep.h $(CPUDIR)/rtinc.h

SOURCES= $(CPUDIR)/dlfcns.c $(CPUDIR)/globals.c $(CPUDIR)/map.c \
	 $(CPUDIR)/rtfcns.c $(CPUDIR)/rtld.c $(CPUDIR)/paths.c  \
	 $(CPUDIR)/reloc.c $(CPUDIR)/rtsetup.c $(CPUDIR)/binder.c 

AS_SOURCES= $(CPUDIR)/rtbinder.s $(CPUDIR)/rtboot.s $(CPUDIR)/rtmemcpy.s \
	 $(CPUDIR)/clrpage.s $(CPUDIR)/align.s $(CPUDIR)/rtstrings.s	\
	 $(CPUDIR)/syscalls.s

# rtboot.o, which contains the startup code, must be linked first
OBJECTS= align.o rtboot.o binder.o dlfcns.o globals.o map.o paths.o reloc.o \
	rtfcns.o rtld.o rtsetup.o rtbinder.o clrpage.o rtsetaddr.o rtmemcpy.o \
	rtstrings.o syscalls.o

PRODUCTS= $(OBJECTS)


all:	$(OBJECTS) 

dlfcns.o:	$(CPUDIR)/dlfcns.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/dlfcns.c

globals.o:	$(CPUDIR)/globals.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/globals.c

map.o:	$(CPUDIR)/map.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/map.c

rtfcns.o:	$(CPUDIR)/rtfcns.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/rtfcns.c

rtld.o:	$(CPUDIR)/rtld.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/rtld.c

paths.o:	$(CPUDIR)/paths.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/paths.c

binder.o:	$(CPUDIR)/binder.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/binder.c

rtsetup.o:	$(CPUDIR)/rtsetup.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/rtsetup.c

reloc.o:	$(CPUDIR)/reloc.c $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(DEFLIST) $(CPUDIR)/reloc.c

rtboot.o:	$(CPUDIR)/rtboot.s $(HFILES)
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/rtboot.s

rtbinder.o:	$(CPUDIR)/rtbinder.s $(HFILES)
	$(CC) -Wa,-m -c $(CFLAGS) $(MODE) $(CPUDIR)/rtbinder.s

clrpage.o:	$(CPUDIR)/clrpage.s
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/clrpage.s

rtmemcpy.o:	$(CPUDIR)/rtmemcpy.s
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/rtmemcpy.s

rtstrings.o:	$(CPUDIR)/rtstrings.s
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/rtstrings.s

syscalls.o:	$(CPUDIR)/syscalls.s 
	$(AS) -o syscalls.o -m -- $(M4DEFS) -DMCOUNT=\# $(LIBCMACH)/nonpic.def $(CPUDIR)/syscalls.s

rtsetaddr.o:	$(CPUDIR)/genset.awk $(CPUDIR)/genset.in $(HFILES)
	$(ECHO) "@FULL" | $(AWK) -f $(CPUDIR)/genset.awk - $(CPUDIR)/genset.in \
		> $(CPUDIR)/rtsetaddr.s
	$(CC) -c $(CFLAGS) $(MODE) $(CPUDIR)/rtsetaddr.s
	
lintit:	$(SOURCES) $(HFILES)
	$(LINT) $(LINTFLAGS) $(LOCALINC) $(DEFLIST) $(SOURCES)

clean:
	-rm -f $(OBJECTS)

clobber:	clean
	-rm -f rtsetaddr.s

print:	$(SOURCES) $(AS_SOURCES) $(HFILES)
	pr -n $(SOURCES) $(AS_SOURCES) $(HFILES) | lp $(PROPTS) 
