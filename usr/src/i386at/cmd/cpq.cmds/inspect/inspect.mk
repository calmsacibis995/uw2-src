#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:inspect/inspect.mk	1.3"

include $(CMDRULES)

MAKEFILE = inspect.mk
BINS = inspect
DIRS = eisa

all:	$(BINS)
	cp inspect.sh inspect
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f $$i.mk $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f $$i.mk $@ ;\
			cd .. ;\
		fi ;\
	done

clean clobber:
	rm -f *.o inspect
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f $$i.mk $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f $$i.mk $@ ;\
			cd .. ;\
		fi ;\
	done

install:	$(BINS)
	-[ -d $(USRBIN)/compaq/inspect ] || mkdir -p $(USRBIN)/compaq/inspect
	$(INS) -f $(USRBIN)/compaq/inspect -m 0700 -u root -g other inspect
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) -f $$i.mk $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) -f $$i.mk $@ ;\
			cd .. ;\
		fi ;\
	done
