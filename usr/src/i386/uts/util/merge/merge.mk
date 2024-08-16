#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)kern-i386:util/merge/merge.mk	1.3"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	merge.mk
KBASE = ../..
DIR = util/merge

MERGE = merge.cf/Driver.o
MODSTUB = merge.cf/Modstub.o

all:	$(MODSTUB)

install:	all
	cd merge.cf; $(IDINSTALL) -R$(CONF) -M merge 

$(MODSTUB): merge_stub.o
	$(LD) -r -o $@ merge_stub.o

clean:
	-rm -f merge_stub.o $(MODSTUB)

clobber:	clean
	-$(IDINSTALL) -e -R$(CONF) -d merge

lintit:

fnames:

sysHeaders = \
	merge386.h

headinstall: $(sysHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

FRC: 

include $(UTSDEPEND)

include $(MAKEFILE).dep
