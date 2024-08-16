#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386sym:mem/mem.mk	1.75"
#ident	"$Header: $"

include $(UTSRULES)

MAKEFILE=	mem.mk
KBASE = ..
LINTDIR = $(KBASE)/lintdir
DIR = mem

MEM = mem.cf/Driver.o
KMA = kma.cf/Driver.o
SEGDEV = segdev.cf/Driver.o
LFILE = $(LINTDIR)/mem.ln

MODULES = \
	$(MEM) \
	$(KMA) \
	$(SEGDEV)

FILES = \
	amp.o \
	copy.o \
	memresv.o \
	move.o \
	pageflt.o \
	pageidhash.o \
	pooldaemon.o \
	rdma.o \
	rff.o \
	seg_dummy.o \
	seg_dz.o \
	seg_kmem.o \
	seg_kvn.o \
	seg_map.o \
	seg_vn.o \
	ublock.o \
	ucopy.o \
	vm_anon.o \
	vm_as.o \
	vm_hat.o \
	vm_hatstatic.o \
	vm_lock.o \
	vm_mapfile.o \
	vm_misc_f.o \
	vm_page.o \
	vm_page_f.o \
	vm_pageout.o \
	vm_pvn.o \
	vm_scalls.o \
	vm_sched.o \
	vm_seg.o \
	vm_swap.o \
	vm_sysinit.o \
	vm_sysinit_p.o \
	vm_vpage.o \
	vm_zbm.o

CFILES = \
	amp.c \
	kma.c \
	drv_mmap.c \
	memresv.c \
	move.c \
	pageflt.c \
	pageidhash.c \
	pooldaemon.c \
	rdma.c \
	rff.c \
	seg_dev.c \
	seg_dummy.c \
	seg_dz.c \
	seg_kmem.c \
	seg_kvn.c \
	seg_map.c \
	seg_vn.c \
	ublock.c \
	ucopy.c \
	vm_anon.c \
	vm_as.c \
	vm_hat.c \
	vm_hatstatic.c \
	vm_lock.c \
	vm_mapfile.c \
	vm_misc_f.c \
	vm_page.c \
	vm_page_f.c \
	vm_pageout.c \
	vm_pvn.c \
	vm_scalls.c \
	vm_sched.c \
	vm_seg.c \
	vm_swap.c \
	vm_sysinit.c \
	vm_sysinit_p.c \
	vm_vpage.c \
	vm_zbm.c

SFILES = \
	copy.s

SRCFILES = $(CFILES) $(SFILES)

LFILES = \
	amp.ln \
	kma.ln \
	drv_mmap.ln \
	memresv.ln \
	move.ln \
	pageflt.ln \
	pageidhash.ln \
	pooldaemon.ln \
	rdma.ln \
	rff.ln \
	seg_dev.ln \
	seg_dummy.ln \
	seg_dz.ln \
	seg_kmem.ln \
	seg_kvn.ln \
	seg_map.ln \
	seg_vn.ln \
	ublock.ln \
	ucopy.ln \
	vm_anon.ln \
	vm_as.ln \
	vm_hat.ln \
	vm_hatstatic.ln \
	vm_lock.ln \
	vm_mapfile.ln \
	vm_misc_f.ln \
	vm_page.ln \
	vm_page_f.ln \
	vm_pageout.ln \
	vm_pvn.ln \
	vm_scalls.ln \
	vm_sched.ln \
	vm_seg.ln \
	vm_swap.ln \
	vm_sysinit.ln \
	vm_sysinit_p.ln \
	vm_vpage.ln \
	vm_zbm.ln

all:	$(MODULES)

install: all
	cd mem.cf; $(IDINSTALL) -R$(CONF) -M mem
	cd kma.cf; $(IDINSTALL) -R$(CONF) -M kma
	cd segdev.cf; $(IDINSTALL) -R$(CONF) -M segdev

$(MEM): $(FILES)
	$(LD) -r -o $(MEM) $(FILES)

$(KMA): kma.o
	$(LD) -r -o $(KMA) kma.o

$(SEGDEV): drv_mmap.o seg_dev.o
	$(LD) -r -o $(SEGDEV) drv_mmap.o seg_dev.o

copy.o: $(KBASE)/util/assym.h $(KBASE)/util/assym_dbg.h

clean:
	-rm -f *.o $(LFILES) *.L

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -d -e mem
	-$(IDINSTALL) -R$(CONF) -d -e kma
	-$(IDINSTALL) -R$(CONF) -d -e segdev

$(LINTDIR):
	-mkdir -p $@

lintit:	$(LFILE)

$(LFILE): $(LINTDIR) $(LFILES)
	-rm -f $(LFILE) `expr $(LFILE) : '\(.*\).ln'`.L
	for i in $(LFILES); do \
		cat $$i >> $(LFILE); \
		cat `basename $$i .ln`.L >> `expr $(LFILE) : '\(.*\).ln'`.L; \
	done

fnames:
	@for i in $(SRCFILES); do \
		echo $$i; \
	done

sysHeaders = \
	immu.h \
	kmem.h \
	lock.h \
	swap.h \
	tuneable.h \
	vmmeter.h \
	vmparam.h
vmHeaders = \
	amp.h \
	anon.h \
	as.h \
	faultcatch.h \
	faultcode.h \
	hat.h \
	kma.h \
	kma_p.h \
	mem_hier.h \
	memresv.h \
	page.h \
	pvn.h \
	rff.h \
	seg.h \
	seg_dev.h \
	seg_dummy.h \
	seg_dz.h \
	seg_kmem.h \
	seg_kvn.h \
	seg_map.h \
	seg_map_u.h \
	seg_vn.h \
	seg_vn_f.h \
	ublock.h \
	vm_hat.h \
	vpage.h \
	zbm.h

headinstall: $(sysHeaders) $(vmHeaders)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
	@-[ -d $(INC)/vm ] || mkdir -p $(INC)/vm
	@for f in $(vmHeaders); \
	 do \
	    $(INS) -f $(INC)/vm -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done

include $(UTSDEPEND)

include $(MAKEFILE).dep
