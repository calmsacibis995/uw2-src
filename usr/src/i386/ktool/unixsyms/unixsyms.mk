#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ktool:i386/ktool/unixsyms/unixsyms.mk	1.11"

include $(CMDRULES)

LDLIBS = $(LIBELF) -lgen

CMDS =	$(SGS)unixsyms

INSDIR = $(CONF)/bin

all:	$(CMDS)

$(SGS)unixsyms: unixsyms.o parseargs.o addsym.o util.o
	$(CC) -o $(SGS)unixsyms unixsyms.o parseargs.o addsym.o util.o \
		$(LDFLAGS) $(LDLIBS) $(ROOTLIBS)

unixsyms.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -D_UTIL_LISTASM_H -D_KMEMUSER  -I$(TARGINC) $(INCLIST)\
	-c unixsyms.c

parseargs.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) -DNATIVE -c parseargs.c

addsym.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -D_UTIL_LISTASM_H -D_KMEMUSER -I$(TARGINC) $(INCLIST)\
	-c addsym.c

util.o: FRC
	$(CC) $(CFLAGS) $(DEFLIST) -I$(TARGINC) $(INCLIST) -c util.c

clean:
	-rm -f *.o

clobber: clean
	-rm -f $(CMDS)

install: all
	-[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	$(INS) -f $(INSDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) $(SGS)unixsyms

lintit:
	$(LINT) $(LINTFLAGS) -I$(TARGINC) $(INCLIST) unixsyms.c addsym.c
	$(LINT) $(LINTFLAGS) $(INCLIST) -DNATIVE parseargs.c

FRC:
