#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:snmp/smux/mib/mib.mk	1.2"

include $(CMDRULES)

MAKEFILE  = mib.mk

MOSY = $(ROOT)/$(MACH)/usr/sbin/mosy

DEFS =  cpqstdeq.mib.defs cpqhost.mib.defs cpqthrsh.mib.defs \
	cpqsinfo.mib.defs cpqida.mib.defs cpqhlth.mib.defs \
	cpqscsi.mib.defs ether.mib.defs token.mib.defs cpqups.mib.defs \
	cpqstsys.mib.defs mib2.txt.defs

MIBS =  cpqstdeq.mib cpqhost.mib cpqthrsh.mib cpqsinfo.mib cpqida.mib \
	cpqhlth.mib cpqscsi.mib ether.mib token.mib cpqups.mib cpqstsys.mib \
	mib2.txt

TARGET = cpqsmuxd.defs

PROBEFILE = mib2.txt
BINARIES = cpqsmuxd.defs

all:
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		for fl in $(BINARIES); do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi

binaries: $(TARGET)

$(TARGET): $(DEFS)  smi.defs 
	cat smi.defs > $(TARGET)
	for MIB in $(MIBS); do \
		cat $$MIB.defs >> $(TARGET); \
	done

cpqstdeq.mib.defs: cpqstdeq.mib 
	$(MOSY) -s $?
cpqhost.mib.defs: cpqhost.mib 
	$(MOSY) -s $?
cpqthrsh.mib.defs: cpqthrsh.mib
	$(MOSY) -s $?
cpqsinfo.mib.defs: cpqsinfo.mib
	$(MOSY) -s $?
cpqida.mib.defs: cpqida.mib
	$(MOSY) -s $?
cpqhlth.mib.defs: cpqhlth.mib 
	$(MOSY) -s $?
cpqscsi.mib.defs: cpqscsi.mib 
	$(MOSY) -s $?
ether.mib.defs: ether.mib 
	$(MOSY) -s $?
token.mib.defs: token.mib 
	$(MOSY) -s $?
cpqups.mib.defs: cpqups.mib 
	$(MOSY) -s $?
cpqstsys.mib.defs: cpqstsys.mib 
	$(MOSY) -s $?
mib2.txt.defs: mib2.txt
	$(MOSY) -s $?

clean:
	rm -f $(DEFS) 

clobber: clean
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

install: all
	-[ -d $(USRBIN)/compaq/smux ] || mkdir -p $(USRBIN)/compaq/smux
	$(INS) -f $(USRBIN)/compaq/smux -m 0700 -u root -g other $(TARGET)
