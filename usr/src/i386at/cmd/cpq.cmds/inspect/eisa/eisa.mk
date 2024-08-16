#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:inspect/eisa/eisa.mk	1.2"

########################################################
# Copyright 1992, 1993 COMPAQ Computer Corporation
########################################################
#
# Title   : $RCSfile: Makefile,v $
#
# Version : $Revision: 2.2 $
#
# Date    : $Date: 1993/02/01 14:56:43 $
#
# Author  : $Author: gregs $
#
########################################################
#
# Change Log :
#
#           $Log: Makefile,v $
# Revision 2.2  1993/02/01  14:56:43  gregs
# Added 1993 to Copyright
#
# Revision 2.1  1992/09/22  20:20:50  andyd
# Release EFS 1.5.0.
#
# Revision 1.1  1992/07/30  22:40:39  andyd
# Initial revision
#
#           $EndLog$
#
########################################################

include $(CMDRULES)

MAKEFILE = eisa.mk

BINS = eisa

PROBEFILE = eisa.c

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINS) \( ! -type f -o -links +1 \) \
			-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) $(BINS) $(MAKEARGS) ;\
	else \
		if [ ! -r $(BINS) ]; then \
			echo "ERROR: $(BINS) is missing" 1>&2 ;\
			false ;\
			break ;\
		fi \
	fi

clean:
	rm -f *.o eisa

clobber:	clean

eisa:	eisa.o
	$(CC) -O -s -o $@ eisa.o

eisa.o:	eisa.c

install:	all
	-[ -d $(USRBIN)/compaq/inspect/I20eisa ] || mkdir -p $(USRBIN)/compaq/inspect/I20eisa
	$(INS) -f $(USRBIN)/compaq/inspect/I20eisa -m 0555 -u bin -g bin eisa
