#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/iiop/iiop.mk	1.5"
#ident	"$Header: $"

#ident	"@(#)kern-i386at:io/hba/iiop/iiop.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	iiop.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/iiop

IIOP = iiop.cf/Driver.o
POLLIIOP = iiop.cf/Driver.o.poll
LFILE = $(LINTDIR)/iiop.ln

FILES = iiop.o
POLLFILES = iiop.poll.o
CFILES = iiop.c 
LFILES = iiop.ln
PROBEFILE = iiop.c
BINARIES = $(POLLIIOP) $(IIOP)

SRCFILES = $(CFILES)

.MUTEX:	$(POLLFILES) $(FILES)

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) "KBASE=$(KBASE)" \
			"LOCALDEF=$(LOCALDEF)" ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -r $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
			false ;\
				break ;\
			fi \
		done \
	fi

binaries :	$(BINARIES)

install:	all
		(cd iiop.cf ; $(IDINSTALL) -R$(CONF) -M iiop; \
		rm -f $(CONF)/pack.d/iiop/disk.cfg;	\
		cp disk.cfg $(CONF)/pack.d/iiop	)

$(POLLIIOP):	$(POLLFILES)
		$(LD) -r -o $(POLLIIOP) $(POLLFILES)

$(IIOP):	$(FILES)
		$(LD) -r -o $(IIOP) $(FILES)

$(POLLFILES):
		$(CC) $(CFLAGS) $(INCLIST) -DIIOP_POLL $(DEFLIST) -c $(CFILES)
		mv $(FILES) $(POLLFILES)

clean:
	-rm -f *.o $(LFILES) *.L

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e iiop
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi


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

Headers = iiop.h

headinstall:
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(Headers); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
