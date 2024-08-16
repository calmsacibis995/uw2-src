#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ihvkit:net/dlpi_ether/dlpi_ether.mk	1.2"
#	Example Makefile for UnixWare 1.0 Network Driver

INC = /usr/include
IDINSTALL = /etc/conf/bin/idinstall
CONF = /etc/conf
INS = /usr/sbin/install
OWN = bin
GRP = bin

DEFLIST = 	-DDL_STRLOG -D_KERNEL -DEL16
INCLIST = 	-I/usr/include -I/usr/include/sys
EL16 =		el16.cf/Driver.o

sysHeaders = \
		./sys/dlpi_ether.h \
		./sys/dlpi_el16.h \
		./sys/el16.h

EL16FILES = \
	dlpi_el16.o \
	el16hrdw.o \
	el16init.o

CFILES =  \
	el16hrdw.c \
	el16init.c 

.c.o:
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c $<

install: headinstall all
	(cd el16.cf; $(IDINSTALL) -R$(CONF) -M el16)

all:	$(EL16)

$(EL16):	$(EL16FILES)
	$(LD) -r -o $@ $(EL16FILES)

#
# Header Install Section
#
headinstall: $(sysHeaders)
	for f in $(sysHeaders);\
	do \
		$(INS) -f $(INC)/sys -m 644 -u $(OWN) -g $(GRP) $$f; \
	done
clean:
	-rm -f *.o $(EL16)

clobber:	clean
	-$(IDINSTALL) -R$(CONF) -e -d el16

#
# Special Header dependencies
#

dlpi_el16.o: 
	$(CC) $(CFLAGS) $(INCLIST) $(DEFLIST) -c dlpi_ether.c && mv dlpi_ether.o dlpi_el16.o
