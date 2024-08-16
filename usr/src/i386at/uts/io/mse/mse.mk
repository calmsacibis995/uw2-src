#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/mse/mse.mk	1.7"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	mse.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = io/mse

LOCALDEF = -DESMP

MSE = mse.cf/Driver.o
LFILE = $(LINTDIR)/mse.ln

FILES = \
	mse.o \
	mse_subr.o

CFILES = \
	mse.c \
	mse_subr.c

LFILES = \
	mse.ln \
	mse_subr.ln

SUBDIRS = smse bmse m320

all:	$(MSE) FRC
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk all"; \
		$(MAKE) -f $$d.mk all $(MAKEARGS)); \
	done

$(MSE): $(FILES)
	$(LD) -r -o $(MSE) $(FILES)

install: $(MSE) FRC
	cd mse.cf; $(IDINSTALL) -R$(CONF) -M mse 
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk install"; \
		$(MAKE) -f $$d.mk install $(MAKEARGS)); \
	done

clean:	localclean
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clean"; \
		$(MAKE) -f $$d.mk clean $(MAKEARGS)); \
	done

localclean:
	-rm -f *.o $(LFILES) *.L $(MSE)

clobber: localclobber
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk clobber"; \
		$(MAKE) -f $$d.mk clobber $(MAKEARGS)); \
	done

localclobber: localclean
	-$(IDINSTALL) -R$(CONF) -d -e mse 

lintit: $(LFILE)
	@for d in $(SUBDIRS); do \
		(cd $$d; echo "=== $(MAKE) -f $$d.mk linit"; \
		$(MAKE) -f $$d.mk lintit $(MAKEARGS)); \
	done

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

sysHeaders = \
	mse.h

headinstall: $(sysHeaders)
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
