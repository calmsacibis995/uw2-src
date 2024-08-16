#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:acc/priv/sum/sum.mk	1.5"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	sum.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = acc/priv/sum

SUM = sum.cf/Driver.o
LFILE = $(LINTDIR)/sum.ln

FILES = \
	sum.o

CFILES = \
	sum.c

SRCFILES = $(CFILES)

LFILES = \
	sum.ln

all:	$(SUM)

install: all
	(cd sum.cf; $(IDINSTALL) -R$(CONF) -M sum)

$(SUM): $(FILES)
	$(LD) -r -o $(SUM) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(SUM)

clobber: clean
	-$(IDINSTALL) -R$(CONF) -d -e sum
	
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
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

headinstall: \
	$(KBASE)/acc/priv/sum/sum.h 
	$(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $(KBASE)/acc/priv/sum/sum.h

include $(UTSDEPEND)

include $(MAKEFILE).dep
