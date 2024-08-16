#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/dlpi_cpq/pnt/pnt.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	pnt.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/dlpi_cpq/pnt
LOCALDEF = -DPNT -DESMP -DENGLISH -DALLOW_SET_EADDR
DLPI_ETHER = $(KBASE)/io/dlpi_ether

PNT = pnt.cf/Driver.o
LFILE = $(LINTDIR)/pnt.ln
PROBEFILE = pnt.c

FILES = \
	dlpi_pnt.o \
	pnt.o

CFILES = \
	pnt.c

LFILES = \
	pnt.ln \
	dlpi_pnt.ln

SRCFILES = $(CFILES) $(DLPI_ETHER)/dlpi_ether.c

.SUFFIXES: .ln

.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-DPNT -c -u $*.c >> $*.L

all: 
	@if [ -f $(PROBEFILE) ]; then \
		find $(PNT) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		if [ ! -r $(PNT) ]; then \
			echo "ERROR: $(PNT) is missing" 1>&2 ;\
			false ;\
			break ;\
		fi \
	fi

install: all
	(cd pnt.cf; $(IDINSTALL) -R$(CONF) -M pnt)

binaries: $(PNT)

$(PNT): $(FILES)
	$(LD) -r -o $(PNT) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d pnt
	@if [ -f $(PROBEFILE) ]; then \
		rm -f $(PNT) ;\
	fi

$(LINTDIR):
	-mkdir -p $@

lintit:
	@if [ -f $(PROBEFILE) ]; then \
		$(MAKE) -f $(MAKEFILE) $(LFILE) $(MAKEARGS) ;\
	fi

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(CFILES); do \
		echo $$i; \
	done

#
# Header Install Section
#

headinstall:

FRC:

include $(UTSDEPEND)

include $(MAKEFILE).dep

#
# Special header dependencies
#
dlpi_pnt.o: $(DLPI_ETHER)/dlpi_ether.c \
	$(KBASE)/fs/buf.h \
	$(KBASE)/fs/buf_f.h \
	$(KBASE)/fs/ioccom.h \
	$(KBASE)/fs/select.h \
	$(KBASE)/io/conf.h \
	$(KBASE)/io/conssw.h \
	$(KBASE)/io/ddi.h \
	$(KBASE)/io/ddi_f.h \
	$(KBASE)/io/dlpi_ether/dlpi_ether.h \
	$(KBASE)/io/f_ddi.h \
	$(KBASE)/io/log/log.h \
	$(KBASE)/io/stream.h \
	$(KBASE)/io/strlog.h \
	$(KBASE)/io/strmdep.h \
	$(KBASE)/io/stropts.h \
	$(KBASE)/io/stropts_f.h \
	$(KBASE)/io/strstat.h \
	$(KBASE)/io/termio.h \
	$(KBASE)/io/termios.h \
	$(KBASE)/io/ttydev.h \
	$(KBASE)/io/uio.h \
	$(KBASE)/mem/immu.h \
	$(KBASE)/mem/kmem.h \
	$(KBASE)/net/dlpi.h \
	$(KBASE)/net/inet/byteorder.h \
	$(KBASE)/net/inet/byteorder_f.h \
	$(KBASE)/net/inet/if.h \
	$(KBASE)/net/inet/strioc.h \
	$(KBASE)/net/socket.h \
	$(KBASE)/net/sockio.h \
	$(KBASE)/proc/cred.h \
	$(KBASE)/proc/disp_p.h \
	$(KBASE)/proc/seg.h \
	$(KBASE)/svc/clock.h \
	$(KBASE)/svc/clock_p.h \
	$(KBASE)/svc/errno.h \
	$(KBASE)/svc/intr.h \
	$(KBASE)/svc/reg.h \
	$(KBASE)/svc/secsys.h \
	$(KBASE)/svc/time.h \
	$(KBASE)/svc/trap.h \
	$(KBASE)/util/cmn_err.h \
	$(KBASE)/util/debug.h \
	$(KBASE)/util/dl.h \
	$(KBASE)/util/engine.h \
	$(KBASE)/util/ipl.h \
	$(KBASE)/util/kdb/kdebugger.h \
	$(KBASE)/util/ksinline.h \
	$(KBASE)/util/ksynch.h \
	$(KBASE)/util/ksynch_p.h \
	$(KBASE)/util/list.h \
	$(KBASE)/util/listasm.h \
	$(KBASE)/util/param.h \
	$(KBASE)/util/param_p.h \
	$(KBASE)/util/sysmacros.h \
	$(KBASE)/util/sysmacros_f.h \
	$(KBASE)/util/types.h \
	$(FRC)
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $(DLPI_ETHER)/dlpi_ether.c && \
		mv dlpi_ether.o dlpi_pnt.o

