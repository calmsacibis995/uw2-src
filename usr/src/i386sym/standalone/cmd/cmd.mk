#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)stand:i386sym/standalone/cmd/cmd.mk	1.1"

include $(UTSRULES)

#must undef GLOBALDEF so we don't have _KERNEL defined
GLOBALDEF =
KBASE = ..
LOCALINC = -I$(INC)
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/cmd.ln
LFILES = boot.ln copy.ln dump.ln prtvtoc.ln CCSformat.ln

SRCS =  boot.c copy.c dump.c prtvtoc.c CCSformat.c
HDRS =  
OBJS =  boot.o copy.o dump.o prtvtoc.o CCSformat.o

all:	$(OBJS)

install: all

clean:
	-/bin/rm -f *.o *.ln *.L

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
