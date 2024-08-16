#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/odi/msm/msm.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE= msm.mk
KBASE 	= ../../..
LINTDIR	= $(KBASE)/lintdir
DIR	= io/msm
MOD 	= msm.cf/Driver.o
LFILE	= $(LINTDIR)/msm.ln
BINARIES = $(MOD)
PROBEFILE = msm.c

#DEBUGDEF= -DDEBUG_TRACE -DNVLT_ModMask=NVLTM_odi
#CC	= epicc -W0,-2N -W0,"-M 0x00020100"	# ODI mask
LOCALDEF= $(DEBUGDEF)
CLOCALDEF= -Kframe $(DEBUGDEF)
CFLAGS	= -O -I$(ODIINC) $(CLOCALDEF)

FILES 	= \
	msmwrap.o \
	msmstrng.o \
	msmioctl.o \
	msmdata.o \
	msmmem.o \
	msmio.o \
	msmos.o \
	msm.o \
	msmfile.o \
	msmglu.o

LFILES	= \
	msmwrap.ln \
	msmstrng.ln \
	msmioctl.ln \
	msmdata.ln \
	msmmem.ln \
	msmio.ln \
	msmos.ln \
	msm.ln \
	msmglu.ln 

CFILES 	= \
	msmwrap.c \
	msmstrng.c \
	msmioctl.c \
	msmdata.c \
	msmmem.c \
	msmio.c \
	msmos.c \
	msm.c 

SRCFILES= $(CFILES)

SFILES 	= \
	msmglu.s 

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

install:all
	(cd msm.cf; $(IDINSTALL) -R$(CONF) -M msm)

clean:
	-rm -f *.o $(LFILES) *.L $(MOD)

clobber:clean
	-$(IDINSTALL) -R$(CONF) -e -d msm
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(LINTDIR):
	-mkaux -p $@

lintit: $(LFILE)

$(LFILE):$(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) :	\
						'\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);	\
	do			\
		echo $$i;	\
	done

binaries: $(BINARIES)

$(BINARIES): $(FILES)
	$(LD) -r -o $@ $(FILES)

msmHeaders = \
	cmsm.h \
	cmsmglu.h \
	msmstruc.h \
	nwctrace2.h \
	odi.h \
	odi_nsi.h \
	odi_portable.h 

headinstall:$(msmHeaders)
	@for f in $(msmHeaders);\
	do			\
		$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN)	\
						-g $(GRP) $$f;	\
	done

include $(UTSDEPEND)

include $(MAKEFILE).dep
