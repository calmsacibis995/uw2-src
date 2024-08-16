#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386sym:io/io.mk	1.81"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	io.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = io

IO = io.cf/Driver.o
LFILE = $(LINTDIR)/io.ln

FILES = \
	autoconf.o \
	conssw.o \
	ddi.o \
	ddi_f.o \
	ddi_misc.o \
	ddi4mp.o \
	ddislp.o \
	dkibind.o \
	metdisk.o \
	physio.o \
	slic.o \
	strcalls.o \
	stream.o \
	streamio.o \
	strsubr.o

CFILES = \
	autoconf.c \
	conssw.c \
	ddi.c \
	ddi_f.c \
	ddi4mp.c \
	ddislp.c \
	dkibind.c \
	metdisk.c \
	physio.c \
	slic.c \
	strcalls.c \
	stream.c \
	streamio.c \
	strsubr.c

SFILES = \
	ddi_misc.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	autoconf.ln \
	conssw.ln \
	ddi.ln \
	ddi_f.ln \
	ddi4mp.ln \
	ddislp.ln \
	dkibind.ln \
	metdisk.ln \
	physio.ln \
	slic.ln \
	strcalls.ln \
	stream.ln \
	streamio.ln \
	strsubr.ln

SUBDIRS = osm sc ssm dlpi_ether wd tm sad ldterm log prf lockstat iaf tp xs \
	mem nullzero uni pipemod gentty clone connld pt ttcompat sp \
	random kbd alp intmap postwait async sysmsg

all:	local FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		 $(MAKE) -f $$d.mk all $(MAKEARGS)); \
	 done

local:	$(IO)

$(IO): $(FILES)
	$(LD) -r -o $(IO) $(FILES)

ddi_misc.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

install: localinstall FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		 $(MAKE) -f $$d.mk install $(MAKEARGS)); \
	 done

localinstall: local FRC
	cd io.cf; $(IDINSTALL) -R$(CONF) -M io

clean:	localclean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		 $(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	 done

localclean:
	-rm -f *.o $(LFILES) *.L

localclobber:	localclean
	-$(IDINSTALL) -R$(CONF) -d -e io

clobber:	localclobber
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		 $(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	 done

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE) FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk lintit"; \
		 $(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	 done

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for d in $(SUBDIRS); do \
	    (cd $$d; \
		$(MAKE) -f $$d.mk fnames $(MAKEARGS) | \
		$(SED) -e "s;^;$$d/;"); \
	done
	@for f in $(SRCFILES); do \
		echo $$f; \
	done

headinstall: localhead FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk headinstall"; \
		 $(MAKE) -f $$d.mk headinstall $(MAKEARGS)); \
	 done

sysHeaders = \
	SGSmem.h \
	SGSproc.h \
	autoconf.h \
	bdp.h \
	cfg.h \
	clkarb.h \
	conf.h \
	conssw.h \
	ddi.h \
	ddi_f.h \
	f_ddi.h \
	intctl.h \
	ioctl.h \
	jioctl.h \
	metdisk.h \
	metdisk_p.h \
	mkdev.h \
	open.h \
	poll.h \
	scan.h \
	scsi.h \
	slic.h \
	slicreg.h \
	stermio.h \
	stream.h \
	strlog.h \
	strmdep.h \
	stropts.h \
	stropts_f.h \
	strstat.h \
	strsubr.h \
	strsubr_f.h \
	strtty.h \
	syslog.h \
	termio.h \
	termios.h \
	termiox.h \
	ttold.h \
	tty.h \
	ttychars.h \
	ttydev.h \
	uio.h

localhead: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC:

include $(UTSDEPEND)

depend::
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk depend";\
		 touch $$d.mk.dep;\
		 $(MAKE) -f $$d.mk depend $(MAKEARGS));\
	done

include $(MAKEFILE).dep
