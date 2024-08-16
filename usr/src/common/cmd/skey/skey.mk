#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)skey:skey.mk	1.13"
#ident	"$Header: $"

include $(CMDRULES)

.SUFFIXES: .p

FRC =
OWN = root
GRP = sys

LINTFLAGS = $(DEFLIST)

MAKEFILE=skey.mk

KEYADMFILES= \
	keyadm.o

GENKEYFILES= \
	genkey.o\
	skeylib.o\
	util.o

HGENKEYFILES = $(GENKEYFILES:.o=.p)

.c.p:
	$(HCC) -Wa,-o,$*.p $(CFLAGS) -I $(INC) -c $<

VALIDATEFILES= \
	validate.o

SFLOP = $(ROOT)/usr/src/$(WORK)/sysinst/desktop/instcmd

MENUDIR = $(ETC)/inst/locale/C/menus/fpk

all: keyadm validate sflop
	if [ -f genkey.c -a -f skeylib.c -a -f util.c ]; then\
		$(MAKE) -f $(MAKEFILE) $(PFX)genkey $(MAKEARGS) ;\
	fi

keyadm: $(KEYADMFILES)
	$(CC) $(LDFLAGS) -o $@ $(KEYADMFILES) $(LDLIBS) $(ROOTLIBS)

$(PFX)genkey: $(HGENKEYFILES)
	$(HCC) $(LDFLAGS) -o $@ $(HGENKEYFILES) $(LDLIBS) $(ROOTLIBS)

genkey: $(GENKEYFILES)
	$(CC) $(LDFLAGS) -o $@ $(GENKEYFILES) $(LDLIBS) $(ROOTLIBS)
	chmod -x genkey

validate: $(VALIDATEFILES)
	$(CC) $(LDFLAGS) -o $@ $(VALIDATEFILES) $(LDLIBS)

sflop: 
	if [ -d $(SFLOP) ]; \
	then \
		$(CC) $(LDFLAGS) -o $@ $(SFLOP)/sflop.c $(LDLIBS) ;\
	 fi

install: all
	$(INS) -f $(SBIN) -m $(BINMODE) -u $(OWN) -g $(GRP) keyadm
	$(INS) -f $(SBIN) -m $(BINMODE) -u $(OWN) -g $(GRP) validate
	$(INS) -f $(SBIN) -m $(BINMODE) -u $(OWN) -g $(GRP) sflop

	rm -rf $(MENUDIR); mkdir $(MENUDIR)
	$(INS) -f $(MENUDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) menu.instr
	$(INS) -f $(MENUDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) menu.errmsg
	$(INS) -f $(MENUDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) menu.final
	$(INS) -f $(MENUDIR) -m $(BINMODE) -u $(OWN) -g $(GRP) menu.choice

	rm -rf $(ETC)/init.d/fpk
	$(INS) -f $(ETC)/init.d -m $(BINMODE) -u $(OWN) -g $(GRP) fpk
	ln $(ETC)/init.d/fpk $(ETC)/rc2.d/S03fpk


clean:
	rm -f *.o *.p

clobber: clean
	rm -f keyadm genkey validate sflop

lint_genkey:
	$(LINT) $(LINTFLAGS) genkey.c util.c skeylib.c

lint_keyadm:
	$(LINT) $(LINTFLAGS) keyadm.c

lint_validate:
	$(LINT) $(LINTFLAGS) validate.c

lintit:
	$(LINT) $(LINTFLAGS) genkey.c util.c skeylib.c
	$(LINT) $(LINTFLAGS) keyadm.c
	$(LINT) $(LINTFLAGS) validate.c

FRC:
