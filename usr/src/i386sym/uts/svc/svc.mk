#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386sym:svc/svc.mk	1.78"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = svc.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = svc

SVC = svc.cf/Driver.o
NAME = name.cf/Driver.o
PSTART = pstart.cf/Driver.o
FPE = fpe.cf/Driver.o
LFILE = $(LINTDIR)/svc.ln

MODULES = \
	$(SVC) \
	$(NAME) \
	$(PSTART) \
	$(FPE) \
	syms.o

# malloc.o is temporary and does not belong here.

FILES = \
	autotune.o \
	bs.o \
	clock.o \
	clock_p.o \
	cxenix.o \
	umem.o \
	sco.o \
	detect.o \
	hrtimers.o \
	intr.o \
	intr_p.o \
	machdep.o \
	main.o \
	memory.o \
	panic.o \
	secsys.o \
	spl.o \
	start.o \
	sysdat.o \
	sysent.o \
	sysi86.o \
	sysi86_p.o \
	sysinit.o \
	timers.o \
	trap.o \
	systrap.o \
	uadmin.o \
	utssys.o \
	xcall.o \
	keyctl.o \
	xsys.o

CFILES = \
	autotune.c \
	bs.c \
	clock.c \
	clock_p.c \
	cxenix.c \
	umem.c \
	sco.c \
	fpe.c \
	hrtimers.c \
	machdep.c \
	main.c \
	memory.c \
	mmu.c \
	name.c \
	panic.c \
	secsys.c \
	sysent.c \
	sysi86.c \
	sysi86_p.c \
	sysinit.c \
	systrap.c \
	timers.c \
	trap.c \
	uadmin.c \
	utssys.c \
	xcall.c \
	keyctl.c \
	xsys.c

SFILES = \
	detect.s \
	intr.s \
	intr_p.s \
	spl.s \
	start.s \
	syms.s \
	syms_p.s \
	sysdat.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	autotune.ln \
	bs.ln \
	clock.ln \
	clock_p.ln \
	cxenix.ln \
	umem.ln \
	sco.ln \
	fpe.ln \
	machdep.ln \
	main.ln \
	memory.ln \
	mmu.ln \
	panic.ln \
	sysent.ln \
	sysi86.ln \
	sysi86_p.ln \
	sysinit.ln \
	systrap.ln \
	timers.ln \
	trap.ln \
	uadmin.ln \
	utssys.ln \
	xcall.ln \
	keyctl.ln \
	xsys.ln

all:	$(MODULES)

install: all
	cd svc.cf; $(IDINSTALL) -R$(CONF) -M svc
	cd pstart.cf; $(IDINSTALL) -R$(CONF) -M pstart
	cd name.cf; $(IDINSTALL) -R$(CONF) -M name
	cd fpe.cf; $(IDINSTALL) -R$(CONF) -M fpe

$(SVC): $(FILES)
	$(LD) -r -o $(SVC) $(FILES)

$(NAME):	name.o
	$(LD) -r -o $(NAME) name.o
	@rm -f name.o
	# remove to force a rebuild every time, to pick up RELEASE, VERSION

name.o: name.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c name.c \
		-DRELEASE=`expr '"$(RELEASE)' : '\(..\{0,8\}\)'`\" \
		-DVERSION=`expr '"$(VERSION)' : '\(..\{0,8\}\)'`\"

name.ln: name.c
	echo "\n$(DIR)/name.c:" > name.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-DRELEASE=`expr '"$(RELEASE)' : '\(..\{0,8\}\)'`\" \
		-DVERSION=`expr '"$(VERSION)' : '\(..\{0,8\}\)'`\" \
		-c -u name.c >> name.L

$(FPE):	fpe.o
	$(LD) -r -o $(FPE) fpe.o

$(PSTART): 16start.o mmu.o
	$(LD) -r -o $(PSTART) 16start.o mmu.o

16start.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

start.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

detect.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

intr_p.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

spl.o:		$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

sysdat.o:	$(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

syms.o:		syms.s syms_p.s $(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h
		$(M4) -DKBASE=$(KBASE) $(DEFLIST) syms.s syms_p.s | \
				$(AS) $(ASFLAGS) -o syms.o -


#	Enhanced Application Compatibility Support

sco.o: sco.c
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c sco.c \
		-DSCODATE=\"`date "+%y/%m/%d"`\"

#	End Enhanced Application Compatibility Support

clean:
	-rm -f *.o $(LFILES) *.L

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e svc
	-$(IDINSTALL) -R$(CONF) -d -e pstart
	-$(IDINSTALL) -R$(CONF) -d -e name

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES) name.ln
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES) name.ln; do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

$(LINTDIR):
	-mkdir -p $@

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	autotune.h \
	bic.h \
	clock.h \
	clock_p.h \
	debugreg.h \
	errno.h \
	fault.h \
	fp.h \
	keyctl.h \
	locking.h \
	p_sysi86.h \
	proctl.h \
	reg.h \
	sd.h \
	secsys.h \
	syscall.h \
	sysi86.h \
	sysconfig.h \
	systeminfo.h \
	systm.h \
	time.h \
	timeb.h \
	trap.h \
	uadmin.h \
	utime.h \
	utsname.h \
	utssys.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
