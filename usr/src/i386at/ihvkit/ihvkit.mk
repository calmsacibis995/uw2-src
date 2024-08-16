#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ihvkit:ihvkit.mk	1.4"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE = ihvkit.mk

INSDIR=$(ROOT)/$(MACH)/usr/src/ihvkit
SUBDIRS = display net pdi
IHVSRC = ihvsrc
HBA = hba
HBADIR = ../uts/io/hba

DDICHK = ddicheck
DDICHKDIR = ../../common/ktool/ddicheck
DDICHKLIST =  \
	$(DDICHKDIR)/README \
	$(DDICHKDIR)/ddicheck \
	$(DDICHKDIR)/ddilint.c \
	$(DDICHKDIR)/ddicheck.mk \
	$(DDICHKDIR)/ddilint.data \
	$(DDICHKDIR)/flint.c
DDICHKINSDIR = $(INSDIR)/pdi/dditools

HBAH = $(HBADIR)/hba.h
DPT = dpt
DPTCFG = dpt.cfg
DPTINSDIR = $(INSDIR)/pdi/$(DPT)
DPTCFGINSDIR = $(INSDIR)/pdi/$(DPT)/dpt.cf
DPTDIR = $(HBADIR)/$(DPT)
DPTCFGDIR = $(DPTDIR)/dpt.cf
DPTLIST = \
	$(DPTDIR)/dpt.c \
	$(DPTDIR)/dpt.h \
	$(DPTDIR)/dpt_sdi.h \
	$(HBAH)

DPTCFGLIST = \
	$(DPTCFGDIR)/Master \
	$(DPTCFGDIR)/Space.c \
	$(DPTCFGDIR)/Drvmap \
	$(DPTCFGDIR)/disk.cfg \
	$(DPTCFGDIR)/System


ICTHA = ictha
ICTHACFG = ictha.cfg
DPTINSDIR = $(INSDIR)/pdi/$(DPT)
ICTHAINSDIR = $(INSDIR)/pdi/$(ICTHA)
ICTHACFGINSDIR = $(INSDIR)/pdi/$(ICTHA)/ictha.cf
ICTHADIR = $(HBADIR)/$(ICTHA)
ICTHACFGDIR = $(ICTHADIR)/ictha.cf
ICTHALIST =  \
	$(ICTHADIR)/ictha.c \
	$(ICTHADIR)/ictha.h \
	$(HBAH)

ICTHACFGLIST = \
	$(ICTHACFGDIR)/Master \
	$(ICTHACFGDIR)/Space.c \
	$(ICTHACFGDIR)/Drvmap \
	$(ICTHACFGDIR)/disk.cfg \
	$(ICTHACFGDIR)/System

MITSUMI = mitsumi
MITSUMICFG = mitsumi.cfg
DPTINSDIR = $(INSDIR)/pdi/$(DPT)
MITSUMIINSDIR = $(INSDIR)/pdi/$(MITSUMI)
MITSUMICFGINSDIR = $(INSDIR)/pdi/$(MITSUMI)/mitsumi.cf
MITSUMIDIR = $(HBADIR)/$(MITSUMI)
MITSUMICFGDIR = $(MITSUMIDIR)/mitsumi.cf
MITSUMILIST =  \
	$(MITSUMIDIR)/mitsumi.c  \
	$(MITSUMIDIR)/mitsumiscsi.c  \
	$(MITSUMIDIR)/hba.c  \
	$(MITSUMIDIR)/scsifake.c  \
	$(MITSUMIDIR)/mitsumi.h \
	$(HBAH)

MITSUMICFGLIST = \
	$(MITSUMICFGDIR)/Master \
	$(MITSUMICFGDIR)/Space.c \
	$(MITSUMICFGDIR)/Drvmap \
	$(MITSUMICFGDIR)/disk.cfg \
	$(MITSUMICFGDIR)/System

all: $(IHVSRC) $(HBA)

$(IHVSRC):
	[ -d $(INSDIR) ] || mkdir -p $(INSDIR); \
		for  i in $(SUBDIRS); do \
			find $$i -type f -follow -print | cpio -pdumL $(INSDIR); \
		done

$(HBA): $(DPT) $(ICTHA) $(MITSUMI) $(DPTCFG) $(ICTHACFG) $(MITSUMICFG) $(DDICHK)

install: all

$(DDICHK):
	for i in $(DDICHKLIST); do \
		cp $$i $(DDICHKINSDIR); \
	done

$(DPT):
	[ -d $(DPTINSDIR) ] || mkdir -p $(DPTINSDIR); \
	for i in $(DPTLIST); do \
		cp $$i $(DPTINSDIR); \
	done

$(DPTCFG):
	for i in $(DPTCFGLIST); do \
		cp $$i $(DPTCFGINSDIR); \
	done
 
$(ICTHA):
	[ -d $(ICTHAINSDIR) ] || mkdir -p $(ICTHAINSDIR); \
	for i in $(ICTHALIST); do \
		cp $$i $(ICTHAINSDIR) ; \
	done

$(ICTHACFG):
	for i in $(ICTHACFGLIST); do \
		cp $$i $(ICTHACFGINSDIR); \
	done

$(MITSUMI):
	[ -d $(MITSUMIINSDIR) ] || mkdir -p $(MITSUMIINSDIR); \
	for i in $(MITSUMILIST); do \
		cp $$i $(MITSUMIINSDIR) ; \
	done

$(MITSUMICFG):
	for i in $(MITSUMICFGLIST); do \
		cp $$i $(MITSUMICFGINSDIR); \
	done

clean:
	rm -rf $(INSDIR)

clobber clean:
