#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)drf:cmd/zip/zip.mk	1.1"

include $(CMDRULES)

ZSOURCE = COPYING Makefile.in alloca.c bits.c configure configure.in \
	  crypt.c crypt.h deflate.c getopt.c getopt.h gzip.c gzip.h \
	  inflate.c lzw.c lzw.h makecrc.c match.S munzip.c mzip.c \
	  revision.h tailor.h trees.c unlzw.c unpack.c unzip.c \
	  util.c zip.c

all: gzip

$(ZSOURCE):
	@ln -s $(PROTO)/cmd/zip/$@ $@

gzip:	$(ZSOURCE)	
	chmod +x configure
	touch configure
	./configure --srcdir=. > /dev/null 2>&1
	$(MAKE)

install: gzip

clean:
	rm -f *.o

clobber: clean
	rm -f $(ZSOURCE) Makefile config.status gzip
