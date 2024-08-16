#ident	"@(#)de_le:common/le/de/build/inst.mk	1.16"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= de

INSTSRC = ../runtime/etc/inst/locale/$(LOCALE)/menus

INSTDIRS = inet/help nics/config nics/help nics/supported_nics/help osmp

HCFFILES = inet/help/genhelp.hcf inet/help/kbhelp.hcf \
	inet/help/net.dnsserv.hcf inet/help/net.frame.hcf \
	inet/help/net.netmask.hcf inet/help/net.routeIP.hcf \
	inet/help/net.sysIP.hcf inet/help/net.broad.hcf \
	inet/help/net.dnsname.hcf \
	nics/help/cable_type.hcf nics/help/dma_value.hcf \
	nics/help/io_addr.hcf nics/help/irq_value.hcf \
	nics/help/mem_addr.hcf nics/help/nic_select.hcf \
	nics/help/phys_mem_type.hcf nics/help/rx_buf_num.hcf \
	nics/help/slot_number.hcf nics/help/tx_buf_num.hcf \
	nics/supported_nics/help/Ansel_NH2100.hcf \
	nics/supported_nics/help/CNET2000T.hcf \
	nics/supported_nics/help/CNET2000T_2.hcf \
	nics/supported_nics/help/CompexEnet16.hcf \
	nics/supported_nics/help/CompexEnet16M.hcf \
	nics/supported_nics/help/DEC203.hcf \
	nics/supported_nics/help/IBM164.hcf  \
	nics/supported_nics/help/IBM164A.hcf \
	nics/supported_nics/help/IBM_AUTOLST_32.hcf \
	nics/supported_nics/help/IBM_EST_32.hcf \
	nics/supported_nics/help/IBM_LST_16.hcf \
	nics/supported_nics/help/IBM_LST_32.hcf \
	nics/supported_nics/help/INT32.hcf \
	nics/supported_nics/help/MD_NE1000BA.hcf \
	nics/supported_nics/help/MD_NE1500TD.hcf \
	nics/supported_nics/help/MD_NE2000A.hcf \
	nics/supported_nics/help/MD_NE2100H.hcf \
	nics/supported_nics/help/MD_NE3200H.hcf \
	nics/supported_nics/help/MD_NTR2000G.hcf \
	nics/supported_nics/help/NE1000.hcf \
	nics/supported_nics/help/NE1500T.hcf \
	nics/supported_nics/help/NE2.hcf \
	nics/supported_nics/help/NE2_32.hcf \
	nics/supported_nics/help/NE2000.hcf \
	nics/supported_nics/help/NE2000Plus.hcf \
	nics/supported_nics/help/NE2100.hcf \
	nics/supported_nics/help/NE3200.hcf \
	nics/supported_nics/help/NS_NE200016AT.hcf \
	nics/supported_nics/help/NS_NE2000Plus.hcf \
	nics/supported_nics/help/NTR2000.hcf \
	nics/supported_nics/help/OC_2121.hcf \
	nics/supported_nics/help/OC_2122.hcf \
	nics/supported_nics/help/OC_2123.hcf \
	nics/supported_nics/help/OCTOK_EISA.hcf \
	nics/supported_nics/help/OCTOK_ISA.hcf \
	nics/supported_nics/help/OCTOK_MCA.hcf \
	nics/supported_nics/help/Oliv_9180.hcf \
	nics/supported_nics/help/Oliv_9181S.hcf \
	nics/supported_nics/help/Oliv_9195.hcf \
	nics/supported_nics/help/RACAL_ES3200.hcf \
	nics/supported_nics/help/RACAL_ES3210TP.hcf \
	nics/supported_nics/help/RACAL_NI6510.hcf \
	nics/supported_nics/help/RACAL_NI9210.hcf \
	nics/supported_nics/help/SMC_8003_I.hcf \
	nics/supported_nics/help/SMC_8003_M.hcf \
	nics/supported_nics/help/SMC_8013_I.hcf \
	nics/supported_nics/help/SMC_8013_M.hcf \
	nics/supported_nics/help/SMC_8100.hcf \
	nics/supported_nics/help/SMC_8100_A.hcf \
	nics/supported_nics/help/SMC_8216.hcf \
	nics/supported_nics/help/SMC_8232.hcf \
	nics/supported_nics/help/SMC_8332.hcf \
	nics/supported_nics/help/SMC_9000.hcf \
	nics/supported_nics/help/TC4045.hcf \
	nics/supported_nics/help/TC4046.hcf \
	nics/supported_nics/help/TCM503.hcf \
	nics/supported_nics/help/TCM509.hcf \
	nics/supported_nics/help/TCM509_E.hcf \
	nics/supported_nics/help/TCM523.hcf \
	nics/supported_nics/help/TCM529.hcf \
	nics/supported_nics/help/TCM579.hcf \
	nics/supported_nics/help/TCM619B.hcf \
	nics/supported_nics/help/TCM629.hcf \
	nics/supported_nics/help/TCM679.hcf \
	nics/supported_nics/help/TOKENBM.hcf \
	nics/supported_nics/help/co596.hcf \
	nics/supported_nics/help/ee16.hcf \
	nics/supported_nics/help/ee16mca.hcf \
	nics/supported_nics/help/el16.hcf \
	nics/supported_nics/help/en596.hcf \
	nics/supported_nics/help/en596e.hcf \
	nics/supported_nics/help/flash32.hcf \
	nics/supported_nics/help/nflxe.hcf \
	nics/supported_nics/help/nflxe_d.hcf \
	nics/supported_nics/help/nflxt.hcf \
	nics/supported_nics/help/nflxt_d.hcf \
	nics/supported_nics/help/pnt.hcf \
	nics/supported_nics/help/pnt_32.hcf \
	nics/supported_nics/help/pnt_ISA.hcf \
	nics/supported_nics/help/pnt_PCI.hcf \
	nics/supported_nics/help/pnt_PNP.hcf

TXTSTRINGS = inet/txtstrings \
	nics/config/Ansel_NH2100 nics/config/CNET2000T \
	nics/config/CNET2000T_2 nics/config/CompexEnet16 \
	nics/config/CompexEnet16M nics/config/DEC203 nics/config/IBM164 \
	nics/config/IBM164A \
	nics/config/IBM_AUTOLST_32 nics/config/IBM_EST_32 \
	nics/config/IBM_LST_16 nics/config/IBM_LST_32 \
	nics/config/INT32 nics/config/MD_NE1000BA \
	nics/config/MD_NE1500TD nics/config/MD_NE2000A nics/config/MD_NE2100H \
	nics/config/MD_NE3200H nics/config/MD_NTR2000G nics/config/NE1000 \
	nics/config/NE1500T nics/config/NE2 nics/config/NE2_32 \
	nics/config/NE2000 nics/config/NE2000Plus nics/config/NE2100 \
	nics/config/NE3200 nics/config/NS_NE200016AT nics/config/NS_NE2000Plus \
	nics/config/NTR2000 nics/config/OC_2121 nics/config/OC_2122 \
	nics/config/OC_2123 nics/config/OCTOK_EISA nics/config/OCTOK_ISA \
	nics/config/OCTOK_MCA nics/config/Oliv_9180 nics/config/Oliv_9181S \
	nics/config/Oliv_9195 nics/config/RACAL_ES3200 \
	nics/config/RACAL_ES3210TP nics/config/RACAL_NI6510 \
	nics/config/RACAL_NI9210 \
	nics/config/SMC_8003_I nics/config/SMC_8003_M nics/config/SMC_8013_I \
	nics/config/SMC_8013_M nics/config/SMC_8100 nics/config/SMC_8100_A \
	nics/config/SMC_8216 nics/config/SMC_8232 nics/config/SMC_8332 \
	nics/config/SMC_9000 nics/config/TC4045 nics/config/TC4046 \
	nics/config/TCM503 nics/config/TCM509 nics/config/TCM509_E \
	nics/config/TCM523 nics/config/TCM529 nics/config/TCM579 \
	nics/config/TCM619B nics/config/TCM629 nics/config/TCM679 \
	nics/config/TOKENBM nics/config/co596 nics/config/ee16 \
	nics/config/ee16mca nics/config/el16 nics/config/pnt \
	nics/config/pnt_32 nics/config/pnt_ISA nics/config/pnt_PCI \
	nics/config/pnt_PNP nics/config/en596 \
	nics/config/en596e nics/config/flash32 nics/config/nflxe \
	nics/config/nflxe_d nics/config/nflxt nics/config/nflxt_d \
	nics/nic_strings \
	osmp/txtstrings

INSTDIR = $(ROOT)/$(MACH)/etc/inst/locale/$(LOCALE)/menus

all:	$(HCFFILES)

hcomp:	hcomp.o wslib.o
	$(HCC) $(CFLAGS) $? -o $@ -lw

hcomp.o: hcomp.c wslib.h
	$(HCC) -I /usr/include -c $(CFLAGS) hcomp.c

wslib.o: wslib.c wslib.h
	$(HCC) -I /usr/include -c $(CFLAGS) wslib.c

$(HCFFILES): hcomp
	@tmp=`basename $(@:.hcf=)` ;\
	cp $(INSTSRC)/$@ $$tmp ;\
	./hcomp $$tmp ;\
	rm -f $$tmp

install: all
	for i in $(INSTDIRS) ;\
	do \
		[ -d $(INSTDIR)/$$i ] || mkdir -p $(INSTDIR)/$$i ;\
	done
	for i in $(HCFFILES) ;\
	do \
		$(INS) -f $(INSTDIR)/`dirname $$i` `basename $$i` ;\
	done
	for i in $(TXTSTRINGS) ;\
	do \
		$(INS) -f $(INSTDIR)/`dirname $$i` $(INSTSRC)/$$i ;\
	done

clean:
	rm -f *.hcf *.o

clobber: clean
	rm -f hcomp

