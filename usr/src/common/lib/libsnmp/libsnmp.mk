#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)libsnmp:libsnmp.mk	1.9"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libsnmp/libsnmp.mk,v 1.7 1994/07/07 18:28:34 cyang Exp $"
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
LOCALINC= -I$(INC)/netmgt
LOCALDEF= -DSVR4 -DSTRNET

LOCALLDFLAGS=-dy -G -ztext
RANLIB	= :
SYS	= svr4

LIBNETMGTDIR = $(ROOT)/$(MACH)/usr/lib

.SUFFIXES: .P

.c.P:
	$(CC) -Wa,-o,$*.P $(CFLAGS) $(DEFLIST) $(PICFLAG) -c $*.c


SMUXSRCS = objects.c smux.c syntax.c
SMUX_O=$(SMUXSRCS:.c=.o)
SMUX_P=$(SMUXSRCS:.c=.P)


SNMPSRCS = asn_smux.c auth_lib.c bld_pkt.c free_lib.c make_lib.c \
	   oid_lib.c prnt_lib.c prse_pkt.c smuxentry.c utilities.c
SNMP_O=$(SNMPSRCS:.c=.o)
SNMP_P=$(SNMPSRCS:.c=.P)

SNMPIOSRCS = smuxio.c snmpio.c
SNMPIO_O=$(SNMPIOSRCS:.c=.o)
SNMPIO_P=$(SNMPIOSRCS:.c=.P)

OBJS = ${SMUX_O} ${SNMP_O} ${SNMPIO_O} ${SMUX_P} ${SNMP_P} ${SNMPIO_P}

all: libsmux.a libsnmp.a libsnmpio.a libsmux.so libsnmp.so libsnmpio.so

libsmux.a: ${SMUX_O}
	$(AR) $(ARFLAGS) $@ $(SMUX_O)

libsnmp.a: ${SNMP_O}
	$(AR) $(ARFLAGS) $@ $(SNMP_O)
	
libsnmpio.a: ${SNMPIO_O}
	$(AR) $(ARFLAGS) $@ $(SNMPIO_O)

libsmux.so: ${SMUX_P}
	$(CC) $(LOCALLDFLAGS) -h /usr/lib/$@ -o $@ $(SMUX_P)

libsnmp.so: ${SNMP_P}
	$(CC) $(LOCALLDFLAGS) -h /usr/lib/$@ -o $@ $(SNMP_P)

libsnmpio.so: ${SNMPIO_P}
	$(CC) $(LOCALLDFLAGS) -h /usr/lib/$@ -o $@ $(SNMPIO_P)

lint: llib-lsmux.ln llib-lsnmp.ln llib-lsnmpio.ln

llib-lsmux.ln: ${SMUXSRCS}
	lint -Csmux ${SMUXSRCS}

llib-lsnmp.ln: ${SNMPSRCS}
	lint -Csnmp ${SNMPSRCS}

llib-lsnmpio.ln: ${SNMPIOSRCS}
	lint -Csnmpio -D${SYS} ${SNMPIOSRCS}

install: all
	@-[ -d $(LIBNETMGTDIR) ] || mkdir -p $(LIBNETMGTDIR)
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








