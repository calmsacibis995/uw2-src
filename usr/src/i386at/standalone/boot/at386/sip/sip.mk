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

#ident	"@(#)stand:i386at/standalone/boot/at386/sip/sip.mk	1.3.1.6"
#ident	"$Header: $"

include $(CMDRULES)

CFLAGS = -O -K no_host

LOCALINC = -I ../.. 

AWK = awk
SED = sed

ASFLAGS = -m
INSDIR = $(ROOT)/$(MACH)/etc/initprog

LOADABLE = sip 
OBJFS = sip.o memsizer.o kjump.o images.o load.o detect.o

CFILES =  \
	sip.c \
	memsizer.c \
	sip_dcmp.c \
	images.c 

all:	$(LOADABLE)

$(LOADABLE): $(OBJFS) ipifile
	${LD} -Mipifile -dn -e sip_start -o $(LOADABLE) $(OBJFS) ;
	../tool/progconf $(LOADABLE) ../mip_pconf.h

ipifile: sipifile ../sip_pconf.h
	cp sipifile ipifile.c
		fgrep NXT_LD_ADDR ../sip_pconf.h | \
		awk '{printf( "define(%s, P%s V%s)\n", $$2, $$3, $$3 )}' >sipifile.m4 ;
		${M4} -D${CCSTYPE} ipifile.c >ipifile ;
	rm -f ipifile.c

kjump.o: kjump.s kjump.m4
	-/bin/rm -f kjump.i
	$(AS) $(ASFLAGS) kjump.s

kjump.m4:     kjump_sym.c ../../boothdr/bootdef.h
	$(CC) $(CFLAGS) $(DEFLIST) $(INCLIST) -S kjump_sym.c
	$(AWK) -f ../syms.awk < kjump_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >kjump.m4

detect.o:
	$(AS) detect.s

clean:
	-/bin/rm -f $(LOADABLE)

clobber: clean
	-/bin/rm -f $(OBJFS) ipifile *.m4 *_sym.s
	-/bin/rm -f $(LOADABLE) ../sip_pconf.h

FRC:
