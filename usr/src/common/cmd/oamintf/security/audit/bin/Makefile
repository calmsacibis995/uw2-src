#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)oamintf:common/cmd/oamintf/security/audit/bin/Makefile	1.2"
#ident "$Header: Makefile 2.0 91/07/12 $"
#
# makefile for validation tests for security oa&m forms
#

include $(CMDRULES)

LDLIBS=-lia
TARGETS=valevt valforloop

all:	$(TARGETS) 

valevt:	valevt.c
	$(CC) $(CFLAGS) $(DEFLIST) -o $(@) $(@).c $(LDFLAGS) $(LDLIBS) $(SHLIBS)
	@echo "--- Created file: $@ ---"

valforloop:	valforloop.sh
	cp $@.sh $@
	@echo "--- Created file: $@ ---"

clean:
	rm -f *.o

clobber: clean
	rm -f $(TARGETS)

lintit:

install: all xinstall

PROTO=../Prototype
OBJECTS=$(TARGETS)
include ../../../Install

