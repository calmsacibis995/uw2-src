#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


# Copyrighted as an unpublished work.
# (c) Copyright 1989 INTERACTIVE Systems Corporation
# All rights reserved.

# RESTRICTED RIGHTS

# These programs are supplied under a license.  They may be used,
# disclosed, and/or copied only as permitted under such license
# agreement.  Any copy must contain the above copyright notice and
# this restricted rights notice.  Use, copying, and/or disclosure
# of the programs is strictly prohibited unless otherwise provided
# in the license agreement.

#ident	"@(#)stand:i386at/standalone/boot/at386/at386.mk	1.6.1.9"
#ident	"$Header: $"

include $(CMDRULES)

LOCALINC = -I ..

SED = sed
AWK = awk
INSDIR = $(ROOT)/$(MACH)/etc

ASFLAGS = -m

HDBOOTST = -DHDTST
BOOTLIB = ../bootlib/bootlib.o

OBJS =  boot.o ix_util.o util.o \
        printf.o gets.o ix_cutil.o  \
	load.o string.o 

CFILES =  \
	boot.c \
	printf.c \
	gets.c \
	ix_cutil.c \
	load.c \
	string.c


SFILES =  \
	ix_util.s \
	util.s

#
#	The following .c.o rule is being defined because we
#	do not with to use all the capabilities of the optimizer
#	Notice we do not include CFLAGS, but set our own
#	first-pass options.  This .c.o rule overides the one
#	in the uts rulefile.

.c.o:
	$(CC) -W0,-Lb -K no_host $(INCLIST) $(DEFLIST) -c $<

# 	fpboot and fdboot make the floppy boot while wpboot and wdboot
#	make the hard disk boot.

LOADABLES = dcmps sips mips 
.MUTEX: $(LOADABLES)

all:	wdboot $(LOADABLES)

depend:: makedep
	-@for i in dcmp mip sip;\
	do\
		if [ -d $$i ]; then\
			cd  $$i;\
			echo "====== $(MAKE) -f $$i.mk depend" ; \
			$(MAKE) -f $$i.mk depend MAKEFILE=$$i.mk $(MAKEARGS); \
			cd .. ; \
		fi;\
	done
	@cd  tool ; \
	echo "====== $(MAKE) -f tools.mk depend" ; \
	$(MAKE) -f tools.mk depend MAKEFILE=tools.mk $(MAKEARGS) 

tools:	
	cd tool; $(MAKE) -f tools.mk $(MAKEARGS)

dcmps:
	cd dcmp; $(MAKE) -f dcmp.mk $(MAKEARGS)

sips:
	cd sip; $(MAKE) -f sip.mk $(MAKEARGS)

mips:
	cd mip; $(MAKE) -f mip.mk $(MAKEARGS)

fpboot: fpriboot.o piftmp tools
	${LD} -Mpiftmp -dn -o fpboot.obj fpriboot.o
	tool/tdxtract fpboot.obj fpboot

cpqpboot: cpqpriboot.o piftmp tools
	${LD} -Mpiftmp -dn -o cpqpboot.obj cpqpriboot.o
	tool/tdxtract cpqpboot.obj cpqpboot

wpboot: wpriboot.o piftmp tools
	${LD} -Mpiftmp -dn -o wpboot.obj wpriboot.o
	tool/tdxtract wpboot.obj wpboot

hdpboot: hdpriboot.o piftmp tools
	${LD} -Mpiftmp -dn -o hdpboot.obj hdpriboot.o
	tool/tdxtract hdpboot.obj hdpboot

cpqfboot: fbootcntl fsboot cpqpboot $(LOADABLES)
	cat cpqpboot fbootcntl fsboot >cpqfboot
	sh tool/sbfedit.sh cpqfboot fsboot.obj

fdboot: fbootcntl fsboot fpboot $(LOADABLES)
	cat fpboot fbootcntl fsboot >fdboot
	sh tool/sbfedit.sh fdboot fsboot.obj

fsboot: siftmp prot.o ftmpdisk.o tools $(OBJS)
	${LD} -Msiftmp -dn -o fsboot.obj prot.o ftmpdisk.o $(OBJS)
	tool/progconf fsboot.obj sbt_pconf.h
	tool/tdxtract fsboot.obj fsboot

wdboot: wbootcntl wsboot wpboot $(LOADABLES)
	cat wpboot wbootcntl wsboot >wdboot

wsboot: siftmp prot.o wtmpdisk.o tools $(OBJS)
	${LD} -Msiftmp -dn -o wsboot.obj prot.o wtmpdisk.o $(OBJS)
	tool/progconf wsboot.obj sbt_pconf.h
	tool/tdxtract wsboot.obj wsboot

#	hdboot is used only for testing hard disk boot without the 
#	need to install the boot program onto the hard disk
#	It is a mix of floppy primary and fixed disk secondary
#	boot programs. It can be loaded onto a floppy and used
#	to start up the kernel on the hard disk.

hdboot: wbootcntl hdsboot hdpboot $(LOADABLES)
	cat hdpboot wbootcntl hdsboot > hdboot

hdsboot: siftmp prot.o hdtmpdisk.o tools $(OBJS)
	${LD} -Msiftmp -dn -o hdsboot.obj prot.o hdtmpdisk.o $(OBJS)
	tool/progconf hdsboot.obj sbt_pconf.h
	tool/tdxtract hdsboot.obj hdsboot

fbootcntl: bootcntl.c tools
	$(CC) -W0,-Lb $(INCLIST) $(DEFLIST) -c bootcntl.c
	${LD} -Mbcifile -dn -o bootcntl.obj bootcntl.o
	tool/tdxtract bootcntl.obj fbootcntl

wbootcntl: bootcntl.c tools
	$(CC) -W0,-Lb $(INCLIST) $(DEFLIST) -DWINI -c bootcntl.c
	${LD} -Mbcifile -dn -o bootcntl.obj bootcntl.o
	tool/tdxtract bootcntl.obj wbootcntl

piftmp:		pribtifile
	${M4} -D${CCSTYPE} pribtifile >piftmp

siftmp:		secbtifile
	${M4} -D${CCSTYPE} secbtifile >siftmp

fpriboot.o:     priboot.s fpriboot.m4
	${AS} $(ASFLAGS) priboot.s
	-/bin/mv priboot.o fpriboot.o

fpriboot.m4: fsboot
	-/bin/rm -f priboot.m4
	$(CC) -W0,-Lb $(INCLIST) $(DEFLIST) -S priboot_sym.c
	$(AWK) -f syms.awk < priboot_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >priboot.m4

wpriboot.o:     priboot.s wpriboot.m4
	${AS} $(ASFLAGS) -- -DWINI priboot.s
	-/bin/mv priboot.o wpriboot.o

wpriboot.m4: wsboot
	-/bin/rm -f priboot.m4
	$(CC) -W0,-Lb $(INCLIST) $(DEFLIST) -DWINI -S priboot_sym.c
	$(AWK) -f syms.awk < priboot_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >priboot.m4

hdpriboot.o:     priboot.s hdpriboot.m4
	${AS} $(ASFLAGS) -- $(HDBOOTST) priboot.s
	-/bin/mv priboot.o hdpriboot.o

hdpriboot.m4: hdsboot
	-/bin/rm -f priboot.m4
	$(CC) -W0,-Lb $(INCLIST) $(HDBOOTST) -S priboot_sym.c
	$(AWK) -f syms.awk < priboot_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >priboot.m4

cpqpriboot.o:     priboot.s fpriboot.m4
	${AS} $(ASFLAGS) -- -DCOMPAQ priboot.s
	-/bin/mv priboot.o cpqpriboot.o

ftmpdisk.o:	fdisk.o $(BOOTLIB)
	${LD} -r -o ftmpdisk.o fdisk.o $(BOOTLIB)

wtmpdisk.o:	wdisk.o ix_alts.o $(BOOTLIB) 
	${LD} -r -o wtmpdisk.o wdisk.o ix_alts.o $(BOOTLIB) 

hdtmpdisk.o:	hddisk.o ix_alts.o $(BOOTLIB) 
	${LD} -r -o hdtmpdisk.o hddisk.o ix_alts.o $(BOOTLIB) 

prot.o: prot.s prot.m4
	${AS} $(ASFLAGS) prot.s

wdisk.o: disk.c				
	${CC} -W0,-Lb ${INCLIST} $(DEFLIST) -DWINI -c disk.c
	mv disk.o wdisk.o

hddisk.o: disk.c				
	${CC} -W0,-Lb -DWINI $(HDBOOTST) ${INCLIST} $(DEFLIST) -c disk.c
	mv disk.o hddisk.o

fdisk.o: disk.c	
	${CC} -W0,-Lb ${INCLIST} $(DEFLIST) -c disk.c
	mv disk.o fdisk.o
prot.m4:
	$(CC) -W0,-Lb $(INCLIST) $(DEFLIST) -S prot_sym.c
	$(AWK) -f syms.awk < prot_sym.s | \
	$(SED) -e '1,$$s;__SYMBOL__;;' >prot.m4

clean:
	-/bin/rm -f *.o *.fd *.hd *.i *iftmp *iftmp.c *.obj *.m4 *_sym.s
	cd dcmp; $(MAKE) -f dcmp.mk clean $(MAKEARGS)
	cd sip; $(MAKE) -f sip.mk clean $(MAKEARGS)
	cd mip; $(MAKE) -f mip.mk clean $(MAKEARGS)
	cd tool; $(MAKE) -f tools.mk clean $(MAKEARGS)

clobber: 
	-/bin/rm -f *.o *.fd *.hd *.i *iftmp *iftmp.c *.obj *.m4 *_sym.s
	-/bin/rm -f ??boot ???boot ?bootcntl sbt_pconf.h
	cd dcmp; $(MAKE) -f dcmp.mk clobber $(MAKEARGS)
	cd sip; $(MAKE) -f sip.mk clobber $(MAKEARGS)
	cd mip; $(MAKE) -f mip.mk clobber $(MAKEARGS)
	cd tool; $(MAKE) -f tools.mk clobber $(MAKEARGS)
	-/bin/rm -f ${INSDIR}/.fboot
	-/bin/rm -f ${INSDIR}/.wboot
	-/bin/rm -f ${INSDIR}/fboot
	-/bin/rm -f ${INSDIR}/boot
	-/bin/rm -f ${INSDIR}/hdboot

FRC: 
