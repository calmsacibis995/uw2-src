#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rsintf.d/restores/restore/Makefile	1.6.7.3"
#ident "$Header: Makefile 1.3 91/05/23 $"

include $(CMDRULES)

SADMDIR=$(USRSADM)
OAMBASE=$(SADMDIR)/sysadm
RESTDIR = $(OAMBASE)/add-ons/bkrs/restores/extended/restore
PRODUCT = restore
OWN=bin
GRP=bin

$(PRODUCT): all

.sh:
	cp $< $*

O_DFILES=Form.restore Text.rest Text.err Help

all: $(O_SHFILES)

clean:

clobber: clean

install: all $(RESTDIR)
	for i in $(O_DFILES) ;\
	do \
		$(INS) -m 644 -g $(GRP) -u $(OWN) -f $(RESTDIR) $$i ;\
	done

size strip lintit:

$(RESTDIR):
	if [ ! -d `dirname $(RESTDIR)` ] ;\
	then \
		mkdir -p `dirname $(RESTDIR)` ;\
	fi
	if [ ! -d $(RESTDIR) ] ;\
	then \
		mkdir -p $(RESTDIR) ;\
	fi
