#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/cpqsc/cpqsc.mk	1.10"

CPQSC = cpqsc.cf/Driver.o
MAKEFILE=	cpqsc.mk
PROBEFILE = cpqsc.c
BINARIES = $(CPQSC)

include $(UTSRULES)

.SUFFIXES: .ln .c710 .out
.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(INCLIST) $(DEFLIST) \
		-c -u $*.c >> $*.L
.out.c710:
	-cat $*.out | col -b | sed 's/\(0[xX][0-9a-fA-F]*\)/(unsigned long)\1/g' > $*.c710

KBASE = ../../..

LOCALDEF = -D_LTYPES -DSYSV -DSVR40 -D_SYSTEMENV=4 -DUNIWARE -DHBA_PREFIX=cpqsc
FS	= $(CONF)/pack.d/cpqsc/Driver.o

GRP = bin
OWN = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM = -m 644 -u root -g sys

sysHeaders = cpqsc.h

HEADERS = cpqU.h

DRVNAME=cpqsc

OBJFILES = \
			hba.o \
			smgrif.o \
			smgrio.o \
			smgrsub.o \
			smgrdbg.o \
			smgrintr.o \
			smgrmem.o \
			cpqsc.o 

CFILES =  \
			hba.c 	\
			cpqsc.c \
			smgrif.c \
			smgrio.c \
			smgrsub.c \
			smgrdbg.c \
			smgrintr.c \
			smgrmem.c

SFILES =  

C710FILES = \
			iop.c710 \
			main.c710 \
			target.c710


LINTFLAGS = -k -n -s
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/cpqsc.ln
LFILES =\
			hba.ln 	\
			cpqsc.ln \
			smgrif.ln \
			smgrio.ln \
			smgrsub.ln \
			smgrdbg.ln \
			smgrintr.ln \
			smgrmem.ln

.s.o:
	$(AS) -m $<

DRVRFILES = cpqsc.cf/Driver.o	

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
	

DRIVER: $(DRVRFILES)

install: all	
	(cd $(DRVNAME).cf; \
	$(IDINSTALL) -R$(CONF) -M $(DRVNAME); \
	$(INS) -f $(CONF)/pack.d/$(DRVNAME) $(INSPERM) disk.cfg )

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
	@for i in $(CFILES); do \
		echo $$i; \
	done

clean:
	rm -f *.o *.c710

clobber: clean
	$(IDINSTALL) -R$(CONF) -e -d $(DRVNAME)
	@if [ -f $(PROBEFILE) ]; then \
                echo "rm -f $(BINARIES)" ;\
                rm -f $(BINARIES) ;\
        fi


$(DRVRFILES):	$(OBJFILES)
	$(LD) -r -o $@ $(OBJFILES)

$(OBJFILES): 
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<

headinstall:	$(sysHeaders)
	@for i in $(sysHeaders); \
	do \
		$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
	done
	@for i in $(HEADERS); \
	do \
		if [ -f $$i ]; then \
			$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
        	fi ;\
	done

FRC: 

smgrmem.o: $(C710FILES)
 
include $(UTSDEPEND)

include $(MAKEFILE).dep
