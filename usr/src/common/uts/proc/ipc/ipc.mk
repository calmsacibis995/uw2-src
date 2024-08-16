#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:proc/ipc/ipc.mk	1.14"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	ipc.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = proc/ipc

IPC = ipc.cf/Driver.o
MODSTUB = ipc.cf/Modstub.o
LFILE = $(LINTDIR)/ipc.ln

IPCFILES = ipc.o msg.o sem.o shm.o

CFILES = \
	ipc.c \
	msg.c \
	sem.c \
	shm.c

SRCFILES = $(CFILES)

LFILES = \
	ipc.ln \
	msg.ln \
	sem.ln \
	shm.ln

.MUTEX:  $(IPC)

all:	$(IPC) $(MODSTUB)

install: all
	cd ipc.cf; $(IDINSTALL) -R$(CONF) -M ipc

$(IPC): $(IPCFILES)
	$(LD) -r -o $(IPC) $(IPCFILES)

$(MODSTUB): ipc_stub.o $(KBASE)/util/mod/stub.m4
	$(LD) -r -o $@ ipc_stub.o

clean:
	-rm -f *.o $(LFILES) *.L $(IPC) $(MODSTUB)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e ipc
	rm -f $(IPC) $(LFILE)

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	ipc.h \
	ipc_f.h \
	ipcsec.h \
	msg.h \
	sem.h \
	shm.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
