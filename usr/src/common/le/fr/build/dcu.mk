#ident	"@(#)fr_le:common/le/fr/build/dcu.mk	1.6"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= fr

DCUSRC = ../runtime/etc/dcu.d/locale/$(LOCALE)

DCUHELP = dcu.addboard.hcf dcu.all.hcf dcu.apply.hcf dcu.board.hcf \
	dcu.brdsum.hcf dcu.bsp.hcf dcu.cancel.hcf dcu.com.hcf dcu.driver.hcf \
	dcu.drivsum.hcf dcu.main.hcf dcu.misc.hcf dcu.network.hcf \
	dcu.restart.hcf dcu.return.hcf dcu.scsi.hcf dcu.sound.hcf \
	dcu.video.hcf dcu.whatis.hcf kbhelp.hcf

DCUCONFIG = config

DCUTXT = txtstrings

DCUDIR = $(ROOT)/$(MACH)/etc/dcu.d/locale/$(LOCALE)

all:	$(DCUTXT) $(DCUCONFIG) $(DCUHELP)

hcomp:	hcomp.o wslib.o
	$(HCC) $(CFLAGS) $? -o $@ -lw

hcomp.o: hcomp.c wslib.h
	$(HCC) -I /usr/include -c $(CFLAGS) hcomp.c

wslib.o: wslib.c wslib.h
	$(HCC) -I /usr/include -c $(CFLAGS) wslib.c

$(DCUTXT):
	cp $(DCUSRC)/$(DCUTXT) .

$(DCUCONFIG): $(DCUTXT)
	cp $(DCUSRC)/config config.sh
	chmod +x ./compile
	./compile ./config.sh

$(DCUHELP): hcomp
	@cp $(DCUSRC)/help/$@ $(@:.hcf=)
	./hcomp $(@:.hcf=)
	@rm -f $(@:.hcf=)

install: all
	[ -d $(DCUDIR)/help ] || mkdir -p $(DCUDIR)/help
	$(INS) -f $(DCUDIR) $(DCUTXT)
	$(INS) -f $(DCUDIR) $(DCUCONFIG)
	for i in $(DCUHELP) ;\
	do \
		$(INS) -f $(DCUDIR)/help $$i ;\
	done
	@rm -f $(DCUDIR)/help/helpwin
	ln -s $(ROOT)/$(MACH)/etc/dcu.d/locale/C/help/helpwin $(DCUDIR)/help/helpwin

clean:
	rm -f $(DCUHELP) $(DCUTXT) $(DCUCONFIG)* *.o

clobber: clean
	rm -f hcomp
