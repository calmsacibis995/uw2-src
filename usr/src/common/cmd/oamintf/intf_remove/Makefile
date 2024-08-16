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

#ident	"@(#)oamintf:common/cmd/oamintf/intf_remove/Makefile	1.5.6.4"
#ident "$Header: Makefile 2.0 91/07/12 $"

include $(CMDRULES)

## libraries used by this process
LIBINTF = ../libintf/libintf.a
LINTFLAGS = -u
LINTLIBS = \
	$(USRLIB)/llib-ladm.ln ../libintf/llib-lintf.ln

## options used to build this command
INCDIR = ../intf_include
LOCALINC = -I $(INCDIR)
LDLIBS = -L ../libintf -lintf -ladm 

MAINS = intf_remove
O_CFILES = remove.o rm_err.o
LINTFILES = remove.c rm_err.c

O_SHFILES=
O_DFILES=r.OAMintf r.OAMmif

.MUTEX:	 all xinstall

all: $(MAINS) $(O_SHFILES) $(O_HELPFILES)

intf_remove: $(O_CFILES)
	$(CC) $(CFLAGS) $(DEFLIST) -o $@ $(O_CFILES) $(LDFLAGS) $(LDLIBS) $(SHLIBS) 
	$(CH)chmod 775 $@

reloc.o: $(INCDIR)/intf.h rm_err.h
rm_err.o: rm_err.h

clean:
	rm -f $(O_CFILES)

clobber: clean
	rm -f $(MAINS) $(O_SHFILES) $(O_HELPFILES)

lintit:
	rm -f lint.out
	for file in $(LINTFILES) ;\
	do \
		$(LINT) $(LINTFLAGS) $(DEFLIST) $$file $(LINTLIBS) >>lint.out ;\
	done


size: all
	$(SIZE) $(MAINS)

strip: all
	$(STRIP) $(MAINS)

install: all xinstall

PROTO=Prototype
OBJECTS=$(MAINS) $(O_SHFILES) $(O_DFILES) $(O_HELPFILES)
include ../Install
