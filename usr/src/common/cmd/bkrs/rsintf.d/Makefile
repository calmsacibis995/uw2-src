#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)bkrs:common/cmd/bkrs/rsintf.d/Makefile	1.1.4.3"
#ident "$Header: Makefile 1.2 91/03/20 $"

include $(CMDRULES)

LOCAL=hdrs

PRODUCTS=restores

all: $(PRODUCTS)

$(PRODUCTS):
	cd $(@); $(MAKE) $(MAKEARGS) all; cd ..

install: all
	for f in $(PRODUCTS) ; \
	do \
		cd $$f ; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

clean:

clobber: clean
	for f in $(PRODUCTS) ; \
	do \
		cd $$f ; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done

size strip lintit:
	for f in $(PRODUCTS) ; \
	do \
		cd $$f ; \
		$(MAKE) $(MAKEARGS) $(@) ; \
		cd ..; \
	done
