#ident	"@(#)fr_le:common/le/fr/build/merge.mk	1.6"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= fr

MDIR	= ../runtime/usr/lib/merge/mergemsg

DDIR	= ../runtime/usr/lib/merge/dosmsg

MERGESRC = $(MDIR)/$(LOCALE).messages $(DDIR)/$(LOCALE).messages

MERGEFILES = $(LOCALE).8859.m $(LOCALE).437.m \
	$(LOCALE).8859.d $(LOCALE).437.d

MERGEDIR = $(ROOT)/$(MACH)/usr/lib/merge

all:	437.m 437.d 8859.m $(MERGESRC) lmfgen
	chmod a+x lmfgen
	./lmfgen 8859.m $(LOCALE).8859.m
	./lmfgen 437.m $(LOCALE).437.m
	./lmfgen $(DDIR)/$(LOCALE).messages $(LOCALE).8859.d
	./lmfgen 437.d $(LOCALE).437.d

437.m:
	iconv -f 88591 -t PC437 $(MDIR)/$(LOCALE).messages | \
		sed -f 437.sed >437.m

437.d:
	iconv -f 88591 -t PC437 $(DDIR)/$(LOCALE).messages >437.d

8859.m:
	csplit $(MDIR)/$(LOCALE).messages /DINFO_/ /DISPLAY_USAGE/ \
		/LCC.MERGE.UNIX.WINSETUP/ /\$quote/
	iconv -f 88591 -t PC850 xx01 >xx1
	mv xx1 xx01
	iconv -f 88591 -t PC850 xx03 >xx3
	sed -f 850.sed xx3 >xx03
	cat xx?? >8859.m
	rm -f xx*

install: all
	[ -d $(MERGEDIR)/mergemsg ] || mkdir -p $(MERGEDIR)/mergemsg
	-cp $(LOCALE).8859.m $(LOCALE).8859
	-cp $(LOCALE).437.m $(LOCALE).437
	$(INS) -f $(MERGEDIR)/mergemsg $(LOCALE).8859
	$(INS) -f $(MERGEDIR)/mergemsg $(LOCALE).437
	-rm -f $(LOCALE).8859 $(LOCALE).437
	[ -d $(MERGEDIR)/dosmsg ] || mkdir -p $(MERGEDIR)/dosmsg
	-cp $(LOCALE).8859.d $(LOCALE).8859
	-cp $(LOCALE).437.d $(LOCALE).437
	$(INS) -f $(MERGEDIR)/dosmsg $(LOCALE).8859
	$(INS) -f $(MERGEDIR)/dosmsg $(LOCALE).437
	-rm -f $(LOCALE).8859 $(LOCALE).437

clean:
	rm -f $(MERGEFILES)

clobber: clean

