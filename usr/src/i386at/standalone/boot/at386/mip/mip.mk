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

#ident	"@(#)stand:i386at/standalone/boot/at386/mip/mip.mk	1.12"
#ident	"$Header: $"

include $(CMDRULES)

CFLAGS = -O -K no_host

LOCALINC = -I ../.. 

INSDIR = $(ROOT)/$(MACH)/etc/initprog

LOADABLE = mip coro_mip

OBJS = mip.o at386.o corollary.o olivetti.o \
	mc386.o intel.o misc386.o

COBJS = coro_mip.o olivetti.o at386.o dell.o coro_corollary.o \
	mc386.o apricot.o intel.o necpm.o misc386.o

CFILES = $(OBJS:.o=.c)

all:	$(LOADABLE)

mip: $(OBJS) ipifile
		${LD} -Mipifile -dn -e mip_start -o $@ $(OBJS)

coro_mip: $(COBJS) ipifile
		${LD} -Mipifile -dn -e mip_start -o $@ $(COBJS)

coro_mip.o:	mip.c
		rm -f coro_mip.c
		ln -s mip.c coro_mip.c
		$(CC) $(CFLAGS) $(INCLIST) -DCOROLLARY -c coro_mip.c
		rm -f coro_mip.c

coro_corollary.o:	corollary.c
		ln -s corollary.c coro_corollary.c
		$(CC) $(CFLAGS) $(INCLIST) -DCOROLLARY -c coro_corollary.c
		rm -f coro_corollary.c

ipifile: mipifile ../mip_pconf.h
	cp mipifile ipifile.c
		fgrep NXT_LD_ADDR ../mip_pconf.h | \
		awk '{printf( "define(%s, P%s V%s)\n", $$2, $$3, $$3 )}' >mipifile.m4 ;
		${M4} -D${CCSTYPE} ipifile.c >ipifile
	rm -f ipifile.c

clean:
	-/bin/rm -f $(LOADABLE)

clobber: clean
	-/bin/rm -f $(OBJS) ipifile *.m4 $(COBJS)
	-/bin/rm -f mip coro_mip

FRC: 
