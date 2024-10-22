#ident	"@(#)ucb:common/ucbcmd/troff/troff.d/devaps/devaps.mk	1.5"
#ident	"$Header: $"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#



#		PROPRIETARY NOTICE (Combined)
#
#This source code is unpublished proprietary information
#constituting, or derived under license from AT&T's UNIX(r) System V.
#In addition, portions of such source code were derived from Berkeley
#4.3 BSD under license from the Regents of the University of
#California.
#
#
#
#		Copyright Notice 
#
#Notice of copyright on this source code product does not indicate 
#publication.
#
#	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
#	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
#	          All rights reserved.

#	makefile for aps-5 driver, fonts, etc.
#
# DSL 2

include $(CMDRULES)

OWN = bin

GRP = bin

INSDIR = $(ROOT)/$(MACH)/usr/ucb
FONTHOME = $(INSDIR)lib/doctools/font
FONTDIR = $(INSDIR)lib/doctools/font/devaps
MAKEDEV = ./makedev
FFILES = [A-Z] [A-Z][0-9A-Z] DESC
OFILES = [A-Z].[oa][ud][td] [A-Z][0-9A-Z].[oa][ud][td] DESC.out

all:	daps aps_fonts

daps:	daps.o ../draw.o build.o
	$(CC) $(LDFLAGS) $(FFLAG) -o daps daps.o ../draw.o build.o -lm

daps.o:	aps.h ../dev.h daps.h daps.g
	$(CC) $(CFLAGS) -I../ -c daps.c

../draw.o:	../draw.c
	cd ..;  $(MAKE) draw.o

aps_fonts:	$(MAKEDEV)
	$(MAKEDEV) DESC
	for i in $(FFILES); \
	do	if [ ! -r $$i.out ] || [ -n "`find $$i -newer $$i.out -print`" ]; \
		   then	$(MAKEDEV) $$i; \
		fi; \
	done
	-if [ -r LINKFILE ]; then \
	    sh ./LINKFILE; \
	fi

$(MAKEDEV):	$(MAKEDEV).c ../dev.h
	$(HCC) -I../ -o $(MAKEDEV) $(MAKEDEV).c
makedir:
	if [ ! -d $(FONTHOME) ] ; then rm -f $(FONTHOME);  mkdir -p $(FONTHOME); \
		chmod 755 $(FONTHOME);  fi
	if [ ! -d $(FONTDIR) ] ; then rm -f $(FONTDIR);  mkdir -p $(FONTDIR); \
		chmod 755 $(FONTDIR);  fi

install: daps aps_fonts makedir
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) daps 
	for i in ${OFILES} version; do \
   	    ($(INS) -f $(FONTDIR) -u $(OWN) -g $(GRP) -m 644 $$i); done

clean:
	rm -f *.o

clobber:	clean
	rm -f $(OFILES) daps makedev
