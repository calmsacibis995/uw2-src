#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)winxksh:winxksh.mk	1.7"

include $(CMDRULES)

DIRS = libwin xksh
TYPE = DYN STATIC

all : $(TYPE)
	@echo "WINXKSH: $(TYPE) build done"

DYN STATIC:
	$(MAKE) -f winxksh.mk clean
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) TYPE=$@ all" ;\
		if cd $$i ;\
		then \
			$(MAKE) TYPE=$@ all ;\
			cd .. ;\
		fi ;\
	done

install strip lintit: all
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) $@ ;\
			cd .. ;\
		fi ;\
	done

clobber clean:
	@for i in $(DIRS) ;\
	do \
		echo "\tcd $$i && $(MAKE) TYPE=DYN $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) TYPE=DYN $@ ;\
			cd .. ;\
		fi ;\
		echo "\tcd $$i && $(MAKE) TYPE=STATIC $@" ;\
		if cd $$i ;\
		then \
			$(MAKE) TYPE=STATIC $@ ;\
			cd .. ;\
		fi ;\
	done
