#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-pdi:io/target/cled/cled.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	cled.mk
KBASE = ../../..
LINTDIR = $(KBASE)/lintdir
DIR = io/target/cled

CLED = cled.cf/Driver.o
LFILE = $(LINTDIR)/cled.ln

BINARIES = $(CLED)
PROBEFILE = cled.c

FILES = cled.o
CFILES = cled.c
LFILES = cled.ln

SRCFILES = $(CFILES)

OBJFILES = $(FILES)

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
	
DRIVER: $(CLED)

install:	all
		(cd cled.cf ; $(IDINSTALL) -R$(CONF) -M cled)

$(CLED):	$(OBJFILES)
	$(LD) -r -o $@ $(OBJFILES)

clean:
	-rm -f $(OBJFILES) $(LFILES) *.L

clobber: clean
	$(IDINSTALL) -R$(CONF) -d -e cled
	@if [ -f $(PROBEFILE) ]; then \
                echo "rm -f $(BINARIES)" ;\
                rm -f $(BINARIES) ;\
        fi


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
	cled.h \
	cledioctl.h \
	cledmsg.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
