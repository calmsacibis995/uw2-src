#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1993 Novell, Inc.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
#	Novell, Inc.   	
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ndt.cmd:ndt.d.mk	1.3"
#ident	"@(#)ndt.d.mk	2.4"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/ndt.d/ndt.d.mk,v 1.2 1994/01/31 21:52:04 duck Exp $"

include $(CMDRULES)

#	Where MAINS are to be installed.
INSDIR = $(USRBIN)

# LOCALDEF = -DIAPX386 -Di386 -DSVR4 -DSVr4 -DSVr4_1 -DIntel_i386 -I/home/duck/p14.1/usr/src/work/uts/util/nuc_tools/trace

#LOCALDEF = -DDBMALLOC -D_KMEMUSER
LOCALDEF = -D_KMEMUSER
LOCALINC = -I/work/usr/src/work/uts

NDTOBJS    = ndtmain.o ndt.o ndtsym.o ndtstat.o ndtstring.o ndtlanz.o util.o vtop.o symtab.o 
MTRACEOBJS = mtrace.o ndtsym.o util.o vtop.o symtab.o 

#all: ndt mtrace
all: ndt

ndt: $(NDTOBJS)
	$(CC) -o ndt $(NDTOBJS) $(LDFLAGS) $(LIBELF)

mtrace: $(MTRACEOBJS)
	$(CC) -o mtrace $(MTRACEOBJS) $(LDFLAGS) $(LIBELF)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	-rm -f $(INSDIR)/ndt
	$(INS) -f $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) ndt
#	-rm -f $(INSDIR)/mtrace
#	$(INS) -f $(INSDIR) -m 0755 -u $(OWN) -g $(GRP) mtrace
	
clean:
	rm -f $(NDTOBJS) $(MTRACEOBJS)
	
clobber: clean
	rm -f ndt

lintit:
	$(LINT) $(LINTFLAGS) ndt.c
	$(LINT) $(LINTFLAGS) ndtmain.c
	$(LINT) $(LINTFLAGS) ndtsym.c
	$(LINT) $(LINTFLAGS) ndtstat.c
	$(LINT) $(LINTFLAGS) ndtstring.c
	$(LINT) $(LINTFLAGS) ndtlanz.c

tags:	FRC
	ctags *.[ch]

FRC:
