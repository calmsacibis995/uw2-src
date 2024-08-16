#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#	Copyrighted as an unpublished work.
#	(c) Copyright 1989 INTERACTIVE Systems Corporation
#	All rights reserved.

#	RESTRICTED RIGHTS

#	These programs are supplied under a license.  They may be used,
#	disclosed, and/or copied only as permitted under such license
#	agreement.  Any copy must contain the above copyright notice and
#	this restricted rights notice.  Use, copying, and/or disclosure
#	of the programs is strictly prohibited unless otherwise provided
#	in the license agreement.

#ident	"@(#)stand:i386at/standalone/boot/at386/dcmp/dcmp.mk	1.4"
#ident	"$Header: $"

include $(CMDRULES)

CFLAGS = -O -K no_host

LOCALINC = -I ../.. -I $(ROOT)/i386at/usr/include

AWK = awk
SED = sed

ASFLAGS = -m
INSDIR = $(ROOT)/$(MACH)/etc/initprog

COMPRESS = zip

LOADABLE = dcmp 
OBJFS = dcmp.o $(COMPRESS).o

CFILES =  \
	dcmp.c

.c.o:
	$(CC) -W0,-Lb -K no_host $(INCLIST) $(DEFLIST) -c $<

all:	$(LOADABLE)

dcmp: $(COMPRESS) $(OBJFS) ipifile
	${LD} -Mipifile -dn -e dcmp_start -o dcmp $(OBJFS) ;
	../tool/progconf dcmp ../sip_pconf.h

$(COMPRESS): FRC
	cd $(COMPRESS); $(MAKE) -f $(COMPRESS).mk CFLAGS='-O -K no_host' all

ipifile: dcmpifile ../sbt_pconf.h
	cp dcmpifile ipifile.c
		fgrep NXT_LD_ADDR ../sbt_pconf.h | \
		awk '{printf( "define(%s, P%s V%s)\n", $$2, $$3, $$3 )}' >dcmpifile.m4 ;
	${M4} -D${CCSTYPE} ipifile.c >ipifile ;
	rm -f ipifile.c

clean:
	-/bin/rm -f $(LOADABLE)

clobber: clean
	-/bin/rm -f $(OBJFS) ipifile *.m4 *_sym.s
	cd $(COMPRESS); $(MAKE) -f $(COMPRESS).mk clobber
	-/bin/rm -f $(LOADABLE) ../sip_pconf.h

FRC:
