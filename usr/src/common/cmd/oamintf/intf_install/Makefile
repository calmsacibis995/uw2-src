#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/intf_install/Makefile	1.6.6.5"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

## libraries used by this process
LIBINTF = ../libintf/libintf.a
LINTLIBS = \
	../libintf/llib-lintf.ln

#	$(USRLIB)/llib-ladm.ln ../libintf/llib-lintf.ln

## options used to build this command
INCDIR = ../intf_include
LOCALINC = -I $(INCDIR)
LDLIBS = -L ../libintf -lintf -ladm 

PROC = mod_menus

O_CFILES = \
	mod_menus.o
O_SHFILES = \
	intf_inst
O_DFILES=\
        i.OAMmif

## objects which make up this process
LINTFILES = \
	mod_menus.c

.MUTEX:	 all xinstall

all: $(O_SHFILES) $(O_DFILES) $(PROC)

$(PROC): $(O_CFILES)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $(O_CFILES) $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	$(CH)chmod 775 $@

mod_menus.o: $(INCDIR)/intf.h $(INCDIR)/menu_io.h $(INCDIR)/inst_err.h

clean:
	rm -f *.o

clobber: clean
	rm -f $(PROC) $(O_SHFILES) $(O_HELPFILES)

size: all
	$(SIZE) $(PROC)

strip: all
	$(STRIP) $(PROC)

lintit:
	rm -f lint.out
	$(LINT) $(DEFLIST) $(LINTFILES) $(LINTLIBS) >>lint.out

install: all xinstall

PROTO=Prototype
OBJECTS=$(PROC) $(O_SHFILES) $(O_DFILES) $(O_HELPFILES)
include ../Install
