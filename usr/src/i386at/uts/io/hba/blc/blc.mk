#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386at:io/hba/blc/blc.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	blc.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/hba/blc

BLC = blc.cf/Driver.o
LFILE = $(LINTDIR)/blc.ln

ASFLAGS = -m

FILES = blc.o
CFILES = blc.c
LFILES = blc.ln

SRCFILES = $(CFILES)

all:
	@if [ -f blc.c ]; then \
                find $(BLC) \( ! -type f -o -links +1 \) \
                        -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
                $(MAKE) -f $(MAKEFILE) $(BLC) $(MAKEARGS) "KBASE=$(KBASE)" \
                        "LOCALDEF=$(LOCALDEF)" ;\
        else \
		if [ ! -r $(BLC) ]; then \
			echo "ERROR: $(BLC) is missing" 1>&2 ;\
			false ;\
		fi \
        fi
	

install:	all
		( \
		cd blc.cf ; $(IDINSTALL) -R$(CONF) -M blc; \
		rm -f  $(CONF)/pack.d/blc/disk.cfg;  \
		cp disk.cfg $(CONF)/pack.d/blc/ \
		)


$(BLC):	$(FILES)
		$(LD) -r -o $(BLC) $(FILES)

clean:
	-rm -f *.o $(LFILES) *.L $(BLC)

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e blc

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


sysHeaders = \
	blc.h

headinstall:
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
		if [ -f $$f ]; then \
		  $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
		fi ;\
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
