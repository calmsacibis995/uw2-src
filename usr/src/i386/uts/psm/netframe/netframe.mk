#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)kern-i386:psm/netframe/netframe.mk	1.2"
#ident	"$Header: $"

include $(UTSRULES)
KBASE = ..

APIC = apic.cf/Driver.o
PIC = pic.cf/Driver.o
PIT = pit.cf/Driver.o
PSM_PSTART = psm_pstart.cf/Driver.o
PSMDRV = psm.cf/Driver.o

MODULES = \
	$(APIC) \
	$(PIC) \
	$(PIT) \
	$(PSM_PSTART) \
	$(PSMDRV)

all:

install: all

$(PSM_PSTART): psm_pstart.o
	$(LD) -r -o $(PSM_PSTART) psm_pstart.o

$(APIC): apic.o 
	$(LD) -r -o $(APIC) apic.o

$(PIC): pic.o intr_p.o spl.o
	$(LD) -r -o $(PIC) pic.o intr_p.o spl.o

$(PIT): pit.o
	$(LD) -r -o $(PIT) pit.o

$(PSMDRV): psm.o clockintr.o nmi.o locks_nodbg.o
	$(LD) -r -o $(PSMDRV) psm.o clockintr.o nmi.o locks_nodbg.o

clean:

clobber: clean

fnames:

sysHeaders =  nf_apic.h  nf_pic.h  nf_pit.h 

headinstall:
