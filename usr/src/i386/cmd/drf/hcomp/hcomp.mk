#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)drf:hcomp/hcomp.mk	1.4"

include $(CMDRULES)

SOURCES = hcomp.c wslib.c wslib.h
HELPFILES = na.hcf drf_help.hcf drf_sh.hcf drf_rst.hcf \
	    drf_mount.hcf drf_umount.hcf drf_snum.hcf drf_rbt.hcf

INSDIR = $(USRLIB)/drf/locale/C

all: hcomp $(HELPFILES)

$(SOURCES):
	@ln -s $(ROOT)/usr/src/$(WORK)/cmd/winxksh/libwin/$@ $@

hcomp:	hcomp.o wslib.o
	$(HCC) $(CFLAGS) $? -o $@ -lw

hcomp.o: hcomp.c wslib.h
	$(HCC) -I/usr/include -c $(CFLAGS) hcomp.c

wslib.o:	wslib.c wslib.h
	$(HCC) -I/usr/include -c $(CFLAGS) wslib.c


$(HELPFILES): $(@:.hcf=)
	./hcomp $(@:.hcf=)

install: all
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR)
	@for i in $(HELPFILES);\
	do \
		$(INS) -f $(INSDIR) $$i ; \
	done
clean:
	rm -f *.o 

clobber: clean
	rm -f hcomp $(SOURCES) $(HELPFILES) 
