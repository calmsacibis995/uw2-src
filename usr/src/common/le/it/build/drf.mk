#ident	"@(#)it_le:common/le/it/build/drf.mk	1.3"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= it

DRFSRC = ../runtime/usr/lib/drf/locale/$(LOCALE)

DRFHELP = drf_help.hcf drf_mount.hcf drf_rbt.hcf drf_rst.hcf drf_sh.hcf \
	drf_snum.hcf drf_umount.hcf na.hcf

DRFTXT = txtstr

DRFDIR = $(ROOT)/$(MACH)/usr/lib/drf/locale/$(LOCALE)

all:	$(DRFTXT) $(DRFHELP)

hcomp:	hcomp.o wslib.o
	$(HCC) $(CFLAGS) $? -o $@ -lw

hcomp.o: hcomp.c wslib.h
	$(HCC) -I /usr/include -c $(CFLAGS) hcomp.c

wslib.o: wslib.c wslib.h
	$(HCC) -I /usr/include -c $(CFLAGS) wslib.c

$(DRFTXT):
	cp $(DRFSRC)/txtstr .

$(DRFHELP): hcomp
	@cp $(DRFSRC)/$@ $(@:.hcf=)
	./hcomp $(@:.hcf=)
	@rm -f $(@:.hcf=)

install: all
	[ -d $(DRFDIR) ] || mkdir -p $(DRFDIR)
	for i in $(DRFHELP) $(DRFTXT) ;\
	do \
		$(INS) -f $(DRFDIR) $$i ;\
	done

clean:
	rm -f $(DRFHELP) $(DRFTXT) *.o

clobber: clean
	rm -f hcomp
