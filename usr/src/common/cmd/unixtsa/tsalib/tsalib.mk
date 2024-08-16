#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)unixtsa:common/cmd/unixtsa/tsalib/tsalib.mk	1.4"
#
# @(#)unixtsa:common/cmd/unixtsa/tsalib/tsalib.mk	1.4	6/7/94
#

TOP=..
include $(TOP)/config.mk

LIB=libsms.a
INSDIR=$(LIBDIR)
TARGET=$(LIBDIR)/$(LIB)

OBJECTS = \
	$(LIB)(crc32unx.o) \
	$(LIB)(datetime.o) \
	$(LIB)(free.o) \
	$(LIB)(hasmntopt.o) \
	$(LIB)(headers.o) \
	$(LIB)(i18n.o) \
	$(LIB)(list.o)	\
	$(LIB)(match.o) \
	$(LIB)(name.o) \
	$(LIB)(parser.o) \
	$(LIB)(parser2.o) \
	$(LIB)(tsalib.o) \
	$(LIB)(str.o) \
	$(LIB)(vstring.o) \
	$(LIB)(strip.o) \
	$(LIB)(portlib.o)

all: $(LIBDIR)/$(LIB)

install : all

clean : do_clean

clobber : clean do_clobber
	$(RM) -f $(LIB) $(LIBDIR)/$(LIB)

$(LIBDIR)/$(LIB) : $(LIB)
	[ -d $(LIBDIR) ] || mkdir -p $(LIBDIR)
	$(CP) -e force $(LIB) $(LIBDIR)

.PRECIOUS: $(LIB) 

# disable this default rule alltogether
.C.a:;

# and use this instead
$(LIB) : $(OBJECTS)
	$(C++C) -c $(CFLAGS) $(DEFLIST) $(INCLIST) $(?:.o=.C)
	$(AR) $(ARFLAGS) $@ $?
	@echo Library $(@F) has been updated.

