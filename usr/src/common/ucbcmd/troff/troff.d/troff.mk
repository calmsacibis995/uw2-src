#ident	"@(#)ucb:common/ucbcmd/troff/troff.d/troff.mk	1.4"
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


#	makefile for (di) troff.  Also builds subproducts - typesetter
#		drivers, fonts, rasters, etc.
#
# DSL 2.

include $(CMDRULES)

OWN = bin

GRP = bin

INCORE = -DINCORE
USG = -DUSG
CFILES=n1.c n2.c n3.c n4.c n5.c t6.c n7.c n8.c n9.c t10.c ni.c nii.c hytab.c suftab.c
HFILES=../tdef.h ../ext.h dev.h
TFILES=n1.o n2.o n3.o n4.o n5.o t6.o n7.o n8.o n9.o t10.o ni.o nii.o hytab.o suftab.o
INSDIR = $(ROOT)/$(MACH)/usr/ucb

all:	troff tmac 

troff:	$(TFILES)
	$(CC) $(LDFLAGS) -o troff $(TFILES) $(PERFLIBS)

n1.o:	../n1.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n1.c
n2.o:	../n2.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n2.c
n3.o:	../n3.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n3.c
n4.o:	../n4.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n4.c
n5.o:	../n5.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(USG) $(INCORE) -c ../n5.c
t6.o:	t6.c ../tdef.h dev.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -I../ -c t6.c
n7.o:	../n7.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n7.c
n8.o:	../n8.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n8.c
n9.o:	../n9.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../n9.c
t10.o:	t10.c ../tdef.h dev.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -I../ -c t10.c
ni.o:	../ni.c ../tdef.h
	$(CC) $(CFLAGS) $(INCORE) -c ../ni.c
nii.o:	../nii.c ../tdef.h ../ext.h
	$(CC) $(CFLAGS) $(INCORE) -c ../nii.c
hytab.o:	../hytab.c
	$(CC) $(CFLAGS) $(INCORE) -c ../hytab.c
suftab.o:	../suftab.c
	$(CC) $(CFLAGS) $(INCORE) -c ../suftab.c

tmac:
	cd tmac.d; $(MAKE) -f tmac.mk INS=$(INS) CH=$(CH)

fonts:	ta aps 

ta:	ta.o draw.o
	$(CC) $(LDFLAGS) $(FFLAG) -o ta ta.o draw.o -lm $(PERFLIBS)

ta.o:	dev.h

aps:	draw.o makedev
	cd devaps;  $(MAKE) -f devaps.mk INS=$(INS) ROOT=$(ROOT) CH=$(CH) \
		CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"

makedev:	makedev.c dev.h
	$(HCC) -o makedev makedev.c $(PERFLIBS)

Dtroff:
	$(MAKE) -f troff.mk troff CFLAGS="$(CFLAGS) -g -DDEBUG" \
		INCORE=$(INCORE) LDFLAGS=-n INS=: CH=#
	mv troff Dtroff

install: troff ta makedev
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) troff 
	$(INS) -f $(INSDIR) -m 755 -u $(OWN) -g $(GRP) ta

	cd devaps;  $(MAKE) -f devaps.mk install INS=$(INS) ROOT=$(ROOT) \
		CH=$(CH) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)"

	cd tmac.d; $(MAKE) -f tmac.mk install INS=$(INS) ROOT=$(ROOT) \
		CH=$(CH) CFLAGS="$(CFLAGS)" LDFLAGS="$(LDFLAGS)" INCORE=$(INCORE)

clean:	taclean
	rm -f $(TFILES) draw.o
	cd devaps;  $(MAKE) -f devaps.mk clean
taclean:  ;  rm -f ta.o

clobber:	taclobber 
	rm -f $(TFILES) draw.o
	rm -f troff makedev
	cd devaps;  $(MAKE) -f devaps.mk clobber
taclobber:	taclean
	rm -f ta
