#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)oampkg:common/cmd/oampkg/mergcont/Makefile	1.1"
#ident "$Header: $"

include $(CMDRULES)

SRCBASE=../
PROC=mergcont
OBJECTS=$(PROC)

## libraries used by this process
LIBINST=../libinst/libinst.a
LINTLIBS=\
	$(USRLIB)/llib-lpkg.ln $(USRLIB)/llib-ladm.ln \
	../libinst/llib-linst.ln

## options used to build this command
LOCALINC=-I. -I ../hdrs
LDLIBS=-L ../libinst -linst -lpkg -ladm -lcmd -lgen

## objects which make up this process
OFILES=\
	main.o

## C files to be linted
LINTFILES=\
	main.c

all:	$(PROC)

$(PROC): $(OFILES) $(LIBINST)
	$(CC) -o $(PROC) $(OFILES) $(LDFLAGS) $(LDLIBS) $(PERFLIBS)
	chmod 775 $(PROC)

clean:
	rm -f $(OFILES)

clobber: clean
	rm -f $(PROC)

lintit:
	$(LINT) $(DEFLIST) $(LINTFILES) $(LINTLIBS) >lint.out 2>&1

include $(SRCBASE)Install
