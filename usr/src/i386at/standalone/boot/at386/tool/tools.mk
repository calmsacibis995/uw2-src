#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.



#ident	"@(#)stand:i386at/standalone/boot/at386/tool/tools.mk	1.3.1.3"
#ident	"$Header: $"

include $(CMDRULES)

GLOBALINC = -I $(ROOT)/i386at/usr/include

LOCALINC = -I ../..

LOCALDEF = -D_KERNEL

LIBELF = $(SGSROOT)/usr/ccs/lib/libelf$(PFX).a

OBJECTS	= tdxtract.o progconf.o sbfedit.o convert.o

MAINS = $(OBJECTS:.o=)

SOURCE = $(OBJECTS:.o=.c)

all: $(OBJECTS) logo.img

install:	all

tdxtract.o progconf.o : 
	$(HCC) $(CFLAGS) $(INCLIST) $(DEFLIST) -o $* $*.c $(LIBELF) -lgen

sbfedit.o : 
	$(HCC) $(CFLAGS) $(INCLIST) $(DEFLIST) -o $* $*.c $(LIBELF) -lgen

convert.o : 
	$(HCC) $(CFLAGS) $(INCLIST) -o $* $*.c

logo.img: convert logo.pcx
	-/bin/rm -f $@
	-./convert logo.pcx logo.img

clean:
	-/bin/rm -f $(MAINS)
	-/bin/rm -f logo.img
	

clobber: clean
	-/bin/rm -fr $(MAINS)

FRC: 
