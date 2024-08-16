#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libnwutil:common/lib/libnutil/test/saptest.mk	1.9"
#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/test/saptest.mk,v 1.17 1994/10/03 14:42:31 mark Exp $"

include $(CMDRULES)

TOP = ../../../cmd/cmd-nw

LOCALINC = \
	-D_REENTRANT

include $(TOP)/local.def

LINTFLAGS = $(GLOBALINC) $(LOCALINC)

SRCS = saptest.c \
	sapadvert.c \
	permad.c \
	sapitest.c \
	thrtest.c

TARGETS = saptest sapitest thrtest permad sapadvert

LDLIBS = -lnwutil -lnsl

all: $(TARGETS)

install:	all

sapadvert:	sapadvert.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

saptest:	saptest.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

sapitest:	sapitest.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

thrtest:	thrtest.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS) -lthread

permad:	permad.o
	$(CC) -o $@ $@.o $(LDFLAGS) $(LDOPTIONS) $(LDLIBS)

clean:
	rm -f *.o

clobber: clean
	rm -f $(TARGETS) lint.out

lintit: $(SRCS)
	-@rm -f lint.out ; \
	for file in $(SRCS) ;\
	do \
	echo '## lint output for '$$file' ##' >>lint.out ;\
	$(LINT) $(LINTFLAGS) $$file $(LINTLIBS) >>lint.out ;\
	done
