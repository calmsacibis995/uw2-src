#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/netmgt/mib/mib.mk	1.5"
#****************************************************************************
#*                                                                          *
#*   Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991, 1992,    *
#*                 1993, 1994  Novell, Inc. All Rights Reserved.            *
#*                                                                          *
#****************************************************************************
#*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	    *
#*      The copyright notice above does not evidence any   	            *
#*      actual or intended publication of such source code.                 *
#****************************************************************************

include $(CMDRULES)

TOP = ../..

include $(TOP)/local.def

MOSY = $(USRSBIN)/mosy

ETC_NM = $(ROOT)/$(MACH)/etc/netmgt

STD_DEFS = smi.defs mibII.defs nm.defs

MAIN_DEFS = unixwared.defs \
		nwumpsd.defs \
		snmpd.defs

UNIXWARE_DEFS = hr.defs

NWUMPS_DEFS = ipx.defs \
		nwuspx.defs \
		nwudiag.defs \
		ripsap.defs

XMODE = 444
OWN = root
GRP = sys

all: $(MAIN_DEFS)

install: all
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) unixwared.defs
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) nwumpsd.defs
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) snmpd.defs
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) hr.mib
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) ipx.mib
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) nm.mib
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) nwudiag.mib
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) nwuspx.mib
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) ripsap.mib
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) mibII.my
	$(INS) -f $(ETC_NM) -m $(XMODE) -u $(OWN) -g $(GRP) smi.my

# Standard Definitions

smi.defs: smi.my
	$(MOSY) -o smi.defs -s smi.my

mibII.defs: mibII.my
	$(MOSY) -o mibII.defs -s mibII.my

nm.defs: nm.mib
	$(MOSY) -o nm.defs -s nm.mib

# NetWare Protocol Stack Definitions

ipx.defs: ipx.mib
	$(MOSY) -o ipx.defs -s ipx.mib

nwudiag.defs: nwudiag.mib
	$(MOSY) -o nwudiag.defs -s nwudiag.mib

nwuspx.defs: nwuspx.mib
	$(MOSY) -o nwuspx.defs -s nwuspx.mib

ripsap.defs: ripsap.mib
	$(MOSY) -o ripsap.defs -s ripsap.mib

nwumpsd.defs: $(NWUMPS_DEFS) $(STD_DEFS)
	cat smi.defs > nwumpsd.defs; \
	cat nm.defs >> nwumpsd.defs; \
	cat ipx.defs >> nwumpsd.defs; \
	cat nwudiag.defs >> nwumpsd.defs; \
	cat nwuspx.defs >> nwumpsd.defs; \
	cat ripsap.defs >> nwumpsd.defs;

# SNMPD Definitions

snmpd.defs: $(NWUMPS_DEFS) $(STD_DEFS)
	cat smi.defs > snmpd.defs; \
	cat mibII.defs >> snmpd.defs; \
	cat nm.defs >> snmpd.defs; \
	cat ipx.defs >> snmpd.defs; \
	cat nwudiag.defs >> snmpd.defs; \
	cat nwuspx.defs >> snmpd.defs; \
	cat ripsap.defs >> snmpd.defs; \
	cat hr.defs >> snmpd.defs;

# UnixWare Definitions

hr.defs: hr.mib
	$(MOSY) -o hr.defs -s hr.mib

unixwared.defs: $(UNIXWARE_DEFS) $(STD_DEFS)
	cat smi.defs > unixwared.defs; \
	cat mibII.defs >> unixwared.defs; \
	cat hr.defs >> unixwared.defs;

clean:
	rm -f $(NWUMPS_DEFS)
	rm -f $(STD_DEFS)
	rm -f $(UNIXWARE_DEFS)

clobber: clean
	rm -f $(MAIN_DEFS)
