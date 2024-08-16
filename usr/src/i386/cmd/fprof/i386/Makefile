#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)fprof:i386/Makefile	1.3"

include $(CMDRULES)

CMDBASE		= ../..
SGSBASE		= ../../sgs
COMDIR		= ../common
CPUDIR		= ../$(CPU)

INS			= $(CMDBASE)/install/install.sh
INSDIR		= $(CCSBIN)

HFILES			= $(COMDIR)/fprof.h $(CPUDIR)/machrel.h
INCDIRS			= -I $(CPUDIR) -I $(COMDIR)
SOURCES			= $(COMDIR)/lib.c $(COMDIR)/fprof.c $(COMDIR)/scan.c $(COMDIR)/stats.c $(COMDIR)/xkfuncs.c
LIB_PRODUCTS	= libfprof.so.1
BIN_PRODUCTS	= fprof lrt_scan
PRODUCTS		= $(LIB_PRODUCTS) $(BIN_PRODUCTS)
LIB_OBJECTS		= lib.o fprof.o stats.o xkfuncs.o
OBJECTS			= scan.o $(LIB_OBJECTS)

SH	= sh
RM	= rm
LN	= ln
CHMOD	= chmod

all: $(PRODUCTS)

profmk: libsymint $(PRODUCTS)

install: all
	for i in $(BIN_PRODUCTS);\
	do \
		cp $$i $$i.bak;\
		$(STRIP) $$i || :;\
		$(SH) $(INS) -f $(CCSBIN) $$i;\
		mv $$i.bak $$i;\
	done
	for i in $(LIB_PRODUCTS);\
	do \
		cp $$i $$i.bak;\
		$(STRIP) $$i;\
		$(SH) $(INS) -f $(CCSLIB) $$i;\
		mv $$i.bak $$i;\
	done

lintit:	$(SOURCES)
	$(LINT) $(LINTFLAGS) $(SOURCES)

clean:
	rm -f $(OBJECTS)

clobber: clean
	rm -f $(PRODUCTS)

libfprof.so.1: $(LIB_OBJECTS)
	$(CC) -h /usr/ccs/lib/libfprof.so.1 $(LDFLAGS) -G -o libfprof.so.1 $(LIB_OBJECTS) -lgen
	$(CHMOD) +x $@

lrt_scan: scan
	$(RM) -f $@
	$(LN) -f $? $@

scan:	scan.o
	$(CC) $(CFLAGS) -o scan scan.o libfprof.so.1

dumplog:	dumplog.o
	$(CC) $(CFLAGS) -o dumplog dumplog.o libfprof.so.1 -lgen

dumplog.o: $(COMDIR)/dumplog.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/dumplog.c

scan.o: $(COMDIR)/scan.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/scan.c

fprof.o: $(COMDIR)/fprof.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/fprof.c

stats.o: $(COMDIR)/stats.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/stats.c

xkfuncs.o: $(COMDIR)/xkfuncs.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/xkfuncs.c

lib.o: $(COMDIR)/lib.c $(COMDIR)/fprof.h
	$(CC) $(CFLAGS) $(INCDIRS) -c $(COMDIR)/lib.c

fprof: $(COMDIR)/fprof.sh
	$(RM) -f $@
	$(CP) $? $@
	$(CHMOD) +x fprof
