#ident	"@(#)ucb:common/ucbcmd/troff/nroff.d/nroff.mk	1.4"
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

#	makefile for new nroff.  Also builds terminal tables.
#

include $(CMDRULES)

INCORE = -DINCORE
USG = -DUSG
LDFLAGS = -s $(SHLIBS)
NROFFLAG = -DNROFF
CFILES=n1.c n2.c n3.c n4.c n5.c n6.c n7.c n8.c n9.c n10.c ni.c nii.c hytab.c suftab.c
HFILES=../tdef.h ../ext.h tw.h
NFILES=n1.o n2.o n3.o n4.o n5.o n6.o n7.o n8.o n9.o n10.o ni.o nii.o hytab.o suftab.o
INSDIR = $(ROOT)/$(MACH)/usr/ucb

OWN = bin

GRP = bin

all:	nroff terms

nroff:	$(NFILES)
	$(CC) $(LDFLAGS) -o nroff $(NFILES)  $(PERFLIBS)

n1.o:	../n1.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I$(INC) -I./ -c ../n1.c
n2.o:	../n2.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I$(INC) -I./ -c ../n2.c
n3.o:	../n3.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n3.c
n4.o:	../n4.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n4.c
n5.o:	../n5.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(USG) $(INCORE) $(NROFFLAG) -c ../n5.c
n6.o:	n6.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I../ -c n6.c
n7.o:	../n7.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n7.c
n8.o:	../n8.c ../ext.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -c ../n8.c
n9.o:	../n9.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../n9.c
n10.o:	n10.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I../ -c n10.c
ni.o:	../ni.c ../tdef.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -c ../ni.c
nii.o:	../nii.c ../tdef.h ../ext.h tw.h
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -I./ -c ../nii.c
hytab.o:	../hytab.c
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -c ../hytab.c
suftab.o:	../suftab.c
	$(CC) $(CFLAGS) $(INCORE) $(NROFFLAG) -c ../suftab.c

terms:
	cd terms.d;  $(MAKE) -f terms.mk all INS=$(INS) ROOT=$(ROOT) CH=$(CH)

Dnroff:
	$(MAKE) -f nroff.mk nroff CFLAGS="$(CFLAGS) -g -DDEBUG" \
		INCORE=$(INCORE) LDFLAGS=-n INS=: CH=#
	mv nroff Dnroff

install: nroff
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) nroff 
	cd terms.d;  $(MAKE) -f terms.mk install INS=$(INS) ROOT=$(ROOT) \
		CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" INCORE=$(INCORE)

clean:
	rm -f $(NFILES)

clobber:	clean
	rm -f nroff
	cd terms.d;  $(MAKE) -f terms.mk clobber
