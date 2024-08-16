#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:net/des/des.mk	1.11"
#ident 	"$Header: $"

include $(UTSRULES)

MAKEFILE=	des.mk
KBASE    = ../..
LINTDIR = $(KBASE)/lintdir
DIR = net/des
LOCALDEF = -DSYSV -UUNIPROC
FRC = 

OFILE = des.cf/Driver.o
DFILE = des.cf/Driver_d.o
IFILE = des.cf/Driver_i.o

DESOBJ =  des_crypt.o des_soft.o des_init.o
IDESOBJ =  intldescrypt.o intldes_soft.o des_init.o

LFILE=$(LINTDIR)/des.ln

FILES = des_crypt.o \
	des_soft.o \
	intldescrypt.o \
	intldes_soft.o \
	des_init.o

LFILES = des_crypt.ln \
	des_soft.ln \
	intldescrypt.ln \
	intldes_soft.ln \
	des_init.ln

CFILES = des_crypt.c \
	des_soft.c \
	intldescrypt.c \
	intldes_soft.c \
	des_init.c

SRCFILES = $(CFILES)

all:
	if [ -s des_crypt.c -a  -s des_soft.c ] ;\
	then \
		$(MAKE) -f des.mk domestic $(MAKEARGS) ;\
	fi
	$(MAKE) -f des.mk intl $(MAKEARGS); \
	rm -f $(OFILE); ln $(IFILE) $(OFILE)


install: all
	(cd des.cf; $(IDINSTALL) -R$(CONF) -M des);
	if [ -s des_crypt.c -a  -s des_soft.c ] ;\
	then \
		cp des.cf/Driver_d.o $(CONF)/pack.d/des; \
		cp des.cf/Driver_i.o $(CONF)/pack.d/des; \
	fi


domestic:       $(DESOBJ)
	$(LD) -r -o $(DFILE) $(DESOBJ)

intl:   $(IDESOBJ)
	$(LD) -r -o $(IFILE) $(IDESOBJ)

clean:
	rm -f *.o $(LFILES) *.L $(OFILE) $(DFILE) $(IFILE)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d des

$(LINTDIR):
	-mkdir -p $@

lintit: $(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

desHeaders = \
	des.h \
	desdata.h \
	softdes.h 

headinstall: $(desHeaders)
	@-[ -d $(INC)/des ] || mkdir -p $(INC)/des
	@for f in $(desHeaders); \
	 do \
	    $(INS) -f $(INC)/des -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done


include $(UTSDEPEND)

include $(MAKEFILE).dep
