#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/fdsb/fdsb.mk	1.2"

include $(UTSRULES)

MAKEFILE=	fdsb.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/fdsb

FDINC = fdinc
FDCOMMON = fdcommon
LOCALINC = -I$(FDINC)
LOCALDEF = -DEHMIO

FDSB = fdsb.cf/Driver.o
LFILE = $(LINTDIR)/fdsb.ln

ASFLAGS = -m

COMMONSRC = fdcio.c osdio.c xxxstrat.c
FILES = ehmio.o fdcio.o osdio.o xxxstrat.o
CFILES = ehmio.c $(COMMONSRC)
LFILES = ehmio.ln fdcio.ln osdio.ln xxxstrat.ln

SRCFILES = $(CFILES)

PROBEFILE = ehmio.c
BINARIES = $(FDSB)

.s.o:
	$(AS) -m $<

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
			-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) DRIVER $(MAKEARGS) "KBASE=$(KBASE)" \
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

.MUTEX: linkcommon $(FDSB)

DRIVER: linkcommon $(FDSB)

install:	all
		( \
		cd fdsb.cf ; $(IDINSTALL) -R$(CONF) -M fdsb; \
		rm -f $(CONF)/pack.d/fdsb/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/fdsb/  \
		)

linkcommon:
	@for i in $(COMMONSRC); do \
		[ -f $$i ] || ln -s $(FDCOMMON)/$$i . ; \
	done

$(FDSB):	$(FILES)
		$(LD) -r -o $(FDSB) $(FILES)


clean:
	-rm -f *.o $(LFILES) *.L $(FDSB)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e fdsb
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi


$(LINTDIR):
	-mkdir -p $@

lintit:	linkcommon $(LFILE)

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
	fdsb.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
