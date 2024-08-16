#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)stand:i386sym/standalone/i386/i386.mk	1.1"

include $(UTSRULES)

#must undef GLOBALDEF so we don't have _KERNEL defined
GLOBALDEF =
KBASE = ..
LOCALINC = -I$(INC)
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/i386.ln
LFILES = lintasm.ln

SRCS =  16start.s bcopy.s bzero.s crt0.s gsp.s symbols.c lintasm.c
OBJS =  16start.o bcopy.o bzero.o crt0.o gsp.o

all:	$(OBJS)

install: all

clean:
	-/bin/rm -f *.o assym.h assym_c.h symbols.s *.ln *.L

clobber: clean

lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

assym.h: symbols.c
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) -S symbols.c
	awk -f symbols.awk symbols.s | \
	sed -e '1,$$s;__SYMBOL__;;' > assym.h
	sed -e 's/^	\.set	\(.*\),\(.*\)/#define	\1	[\2]/' \
		assym.h > assym_c.h

bcopy.o         : bcopy.s assym.h asm.m4

bzero.o         : bzero.s assym.h asm.m4

crt0.o          : crt0.s assym.h asm.m4

gsp.o           : gsp.s assym.h asm.m4
