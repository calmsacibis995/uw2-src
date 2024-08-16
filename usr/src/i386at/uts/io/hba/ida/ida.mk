#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/ida/ida.mk	1.15"
#ident	"$Header: $"

IDA = ida.cf/Driver.o
MAKEFILE = ida.mk
PROBEFILE = idascsi.c 
BINARIES = $(IDA)

include $(UTSRULES)

.SUFFIXES: .ln
.c.ln:
	echo "\n$(DIR)/$*.c:" > $*.L
	-$(LINT) $(LINTFLAGS) $(CFLAGS) $(INCLIST) $(DEFLIST) \
		-c -u $*.c >> $*.L

KBASE = ../../..

LOCALDEF = -D_LTYPES -DSYSV -DSVR40 -D_SYSTEMENV=4 
#-DIDA_DEBUG0 -DIDA_DEBUG_ASSERT -DIDADBG

FS	= $(CONF)/pack.d/ida/Driver.o

GRP = bin
OWN = bin
HINSPERM = -m 644 -u $(OWN) -g $(GRP)
INSPERM = -m 644 -u root -g sys

HEADERS =  \
	  ida.h idaM.h

DRVNAME=ida

OBJFILES = \
	ida.o \
	hba.o \
	scsifake.o \
	idascsi.o 

CFILES =  \
	ida.c \
	hba.c \
	scsifake.c \
	idascsi.c 

SFILES =  

LINTFLAGS = -k -n -s
LINTDIR = $(KBASE)/lintdir
LFILE = $(LINTDIR)/ida.ln
LFILES = \
	ida.ln \
	hba.ln \
	scsifake.ln \
	idascsi.ln

.s.o:
	$(AS) -m $<

DRVRFILES = ida.cf/Driver.o

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


clean:
	-rm -f *.o $(LFILES) *.L

clobber: clean
	$(IDINSTALL) -R$(CONF) -e -d $(DRVNAME)
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

$(DRVRFILES):	$(OBJFILES)
	$(LD) -r -o $@ $(OBJFILES)

$(OBJFILES): 
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST)  -c $<

headinstall:	
	@for i in $(HEADERS); \
	do \
		if [ -f $$i ]; then \
			$(INS) -f $(INC)/sys $(HINSPERM) $$i; \
		fi ;\
	done

FRC: 
 
include $(UTSDEPEND)

include $(MAKEFILE).dep
