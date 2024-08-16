#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.lib/libsnmp/libsnmp.mk	1.4"
#      @(#)libsnmp.mk	1.1 STREAMWare TCP/IP SVR4.2  source
#      SCCS IDENTIFICATION
#      @(#)snmp.mk	6.3 INTERACTIVE SNMP  source
#
# Copyrighted as an unpublished work.
# (c) Copyright 1989 INTERACTIVE Systems Corporation
# All rights reserved.
#


# Copyright (c) 1987, 1988 Kenneth W. Key and Jeffrey D. Case

include $(LIBRULES)
LOCALINC= -I../../head/netmgt
LOCALDEF= -DSVR4 -DSTRNET $(PICFLAG) 

LOCALLDFLAGS=-dy -G -ztext
RANLIB	= :
SYS	= svr4

SMUXSRCS = objects.c smux.c syntax.c
SMUXOBJS = objects.o smux.o syntax.o

SNMPSRCS = asn_smux.c auth_lib.c bld_pkt.c free_lib.c make_lib.c \
	   oid_lib.c prnt_lib.c prse_pkt.c smuxentry.c utilities.c
SNMPOBJS = asn_smux.o auth_lib.o bld_pkt.o free_lib.o make_lib.o \
	   oid_lib.o prnt_lib.o prse_pkt.o smuxentry.o utilities.o

SNMPIOSRCS = smuxio.c snmpio.c
SNMPIOOBJS = smuxio.o snmpio.o

OBJS = ${SMUXOBJS} ${SNMPOBJS} ${SNMPIOOBJS}

all: libsmux.a libsnmp.a libsnmpio.a libsmux.so libsnmp.so libsnmpio.so

libsmux.a: ${SMUXOBJS}
	ar cr libsmux.a $(SMUXOBJS)

libsnmp.a: ${SNMPOBJS}
	ar cr libsnmp.a $(SNMPOBJS)
	
libsnmpio.a: ${SNMPIOOBJS}
	ar cr libsnmpio.a $(SNMPIOOBJS)

libsmux.so: ${SMUXOBJS}
	$(CC) $(LOCALLDFLAGS) -h /usr/lib/$@ -o $@ $(SMUXOBJS)

libsnmp.so: ${SNMPOBJS}
	$(CC) $(LOCALLDFLAGS) -h /usr/lib/$@ -o $@ $(SNMPOBJS)

libsnmpio.so: ${SNMPIOOBJS}
	$(CC) $(LOCALLDFLAGS) -h /usr/lib/$@ -o $@ $(SNMPIOOBJS)

lint: llib-lsmux.ln llib-lsnmp.ln llib-lsnmpio.ln

llib-lsmux.ln: ${SMUXSRCS}
	lint -Csmux ${SMUXSRCS}

llib-lsnmp.ln: ${SNMPSRCS}
	lint -Csnmp ${SNMPSRCS}

llib-lsnmpio.ln: ${SNMPIOSRCS}
	lint -Csnmpio -D${SYS} ${SNMPIOSRCS}

install: all
	$(INS) -f $(USRLIB) -m 0755 -u $(OWN) -g $(GRP) libsmux.so
	$(INS) -f $(USRLIB) -m 0755 -u $(OWN) -g $(GRP) libsnmp.so
	$(INS) -f $(USRLIB) -m 0755 -u $(OWN) -g $(GRP) libsnmpio.so
	$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) libsmux.a
	$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) libsnmp.a
	$(INS) -f $(USRLIB) -m 0444 -u $(OWN) -g $(GRP) libsnmpio.a

clean:
	rm -f ${OBJS} *~ *.so *.a

clobber: clean
	rm -f libsmux.so libsnmp.so libsnmpio.so llib-lsmux.ln llib-lsnmp.ln llib-lsnmpio.ln

#
# Dependencies
#
objects.o: objects.c $(INC)/netmgt/objects.h $(INC)/netmgt/snmp.h
smux.o: smux.c $(INC)/netmgt/snmp.h
syntax.o: syntax.c $(INC)/netmgt/objects.h $(INC)/netmgt/snmp.h

asn_smux.o: asn_smux.c $(INC)/netmgt/snmp.h
auth_lib.o: auth_lib.c $(INC)/netmgt/snmp.h
bld_pkt.o: bld_pkt.c $(INC)/netmgt/snmp.h
free_lib.o: free_lib.c $(INC)/netmgt/snmp.h
make_lib.o: make_lib.c $(INC)/netmgt/snmp.h
oid_lib.o: oid_lib.c $(INC)/netmgt/snmp.h $(INC)/netmgt/snmpuser.h $(INC)/netmgt/snmp-mib.h
prnt_lib.o: prnt_lib.c $(INC)/netmgt/snmp.h $(INC)/netmgt/snmpuser.h
prse_pkt.o: prse_pkt.c $(INC)/netmgt/snmp.h
smuxentry.o: smuxentry.c $(INC)/netmgt/snmp.h
utilities.o: utilities.c $(INC)/netmgt/snmp.h

smuxio.o: smuxio.c
snmpio.o: snmpio.c $(INC)/netmgt/snmp.h $(INC)/netmgt/snmpuser.h $(INC)/netmgt/snmpio.h $(INC)/netmgt/snmpio.${SYS}.h








