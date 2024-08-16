#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern:fs/namefs/namefs.mk	1.9"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	namefs.mk
KBASE = ../..
LINTDIR = $(KBASE)/lintdir
DIR = fs/namefs

NAMEFS = namefs.cf/Driver.o
LFILE = $(LINTDIR)/namefs.ln

MODULES = \
	$(NAMEFS)

FILES = namesubr.o \
	namevfs.o \
	namevnops.o

LFILES = namesubr.ln \
	namevfs.ln \
	namevnops.ln

CFILES = namesubr.c \
	namevfs.c \
	namevnops.c

SRCFILES = $(CFILES)

all:	$(MODULES)

install: all
	(cd namefs.cf; $(IDINSTALL) -R$(CONF) -M namefs)

$(NAMEFS):	$(FILES)
	$(LD) -r -o $(NAMEFS) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(NAMEFS)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e namefs

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
	@for i in $(SRCFILES);	\
	do \
		echo $$i; \
	done

sysfsHeaders = \
	namenode.h

headinstall: $(sysfsHeaders)
	@-[ -d $(INC)/sys/fs ] || mkdir -p $(INC)/sys/fs
	@for f in $(sysfsHeaders); \
	 do \
	    $(INS) -f $(INC)/sys/fs -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
