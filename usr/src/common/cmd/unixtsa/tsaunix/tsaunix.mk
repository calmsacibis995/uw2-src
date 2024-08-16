#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsaunix/tsaunix.mk	1.3"
#
# @(#)unixtsa:common/cmd/unixtsa/tsaunix/tsaunix.mk	1.3	2/19/94
#

TOP=..
include $(TOP)/config.mk

INSDIR=$(BINDIR)
TARGET=tsaunix
LDLIBS=-lcrypt -lgen

OBJECTS = \
	backup1.o \
	backup2.o \
	backup3.o \
	encrypt.o \
	filesys.o \
	hardlink.o \
	isvalid.o \
	mapusers.o \
	read.o \
	respond.o \
	restore1.o \
	restore2.o \
	restore3.o \
	scanutil.o \
	sequtil1.o \
	sequtil2.o \
	smsapi.o \
	smspcall.o \
	tsamain.o \
	tsamsgs.o \
	tsapis.o \
	tsasmsp.o \
	write.o

all: $(TARGET)

install : $(BINDIR)/$(TARGET)

clean : do_clean

clobber : clean do_clobber

$(TARGET) : $(OBJECTS) ../lib/libsms.a
	$(C++C) $(CFLAGS) -o $@ $(OBJECTS) $(LIBLIST)

$(BINDIR)/$(TARGET) : $(TARGET)
	[ -d $(BINDIR) ] || mkdir -p $(BINDIR)
	$(INS) -f $(BINDIR) -g $(GRP) -u $(OWN) -m $(BINMODE) $(TARGET)
