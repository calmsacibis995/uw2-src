#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:snmp/smux/smux.mk	1.3"

include $(CMDRULES)

MAKEFILE = smux.mk

DIRS =  parse mib

SNMPLIBS = -lsmux -lsnmp -lsnmpio
LDLIBS   = $(SNMPLIBS) parse/libparse.a ../lib/libmon.a -lsocket -lnsl -lc

LOCALINC = -I../define -I../lib -I./parse -I.
LOCALDEF = -Dfprintf=agentlog_fprintf -DCOMPAQ

LDFLAGS	= -s -Bdynamic

BINS = cpqsmuxd
BINARIES = cpqsmuxd
PROBEFILE = smuxd.c 
BINARIES = $(BINS)

OBJECTS = smuxd.o mib.o lib.o router.o 

all: 
	(cd parse; $(MAKE) -f parse.mk all)
	(cd mib; $(MAKE) -f mib.mk all)
	> agenterrs.log
	@if [ -f $(PROBEFILE) ]; then \
		find $(BINARIES) \( ! -type f -o -links +1 \) \
		    -exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) binaries $(MAKEARGS) ;\
	else \
		for fl in $(BINARIES) smuxmgr.etc ; do \
			if [ ! -f $$fl ]; then \
				echo "ERROR: $$fl is missing" 1>&2 ;\
				false ;\
				break ;\
			fi \
		done \
	fi


binaries: $(BINARIES)

$(BINS):  $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS) $(LDLIBS)

install:  all
	(cd parse; $(MAKE) -f parse.mk install)
	(cd mib; $(MAKE) -f mib.mk install)
	-[ -d $(USRBIN)/compaq/smux ] || mkdir -p $(USRBIN)/compaq/smux
	-[ -d $(ROOT)/$(MACH)/var/spool/compaq/smux ] || mkdir -p $(ROOT)/$(MACH)/var/spool/compaq
	$(INS) -f $(USRBIN)/compaq/smux -m 0700 -u root -g other cpqsmuxd
	$(INS) -f $(USRBIN)/compaq/smux -m 0700 -u root -g other smuxmgr.etc
	$(INS) -f $(ROOT)/$(MACH)/var/spool/compaq -m 0700 -u root -g other agenterrs.log

clean: 
	(cd parse; $(MAKE) -f parse.mk clean)
	(cd mib; $(MAKE) -f mib.mk clean)
	rm -f *.o core agenterrs.log

clobber:  clean
	(cd parse; $(MAKE) -f parse.mk clobber)
	(cd mib; $(MAKE) -f mib.mk clobber)
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(BINARIES)" ;\
		rm -f $(BINARIES) ;\
	fi

lintit:
	:	
.ALWAYS:
